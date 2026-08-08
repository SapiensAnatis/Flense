#pragma once

#include "ChunkPool.h"
#include "FilesystemTree.h"
#include "ThreadPool.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Flense::Core
{
    class ArchiveEntry;
    class ImageLayer;

    /// <summary>
    /// Configuration for an ImageParser.
    /// </summary>
    struct ImageParserOptions
    {
        /// <summary>
        /// How many threads decompress layer blobs. 0 means one per hardware thread; 1 means the work
        /// happens inline on the thread calling ProcessEntry, straight off the archive, with no pool,
        /// no read-ahead and no buffering.
        /// </summary>
        size_t workerCount = 0;

        /// <summary>
        /// The ceiling on how many bytes of layer data may be buffered between the archive walk and
        /// the workers. This is a hard cap: layers stream through it a chunk at a time, so a layer
        /// larger than the whole budget costs no more memory than a small one.
        /// </summary>
        uint64_t readAheadByteBudget = 256ull * 1024 * 1024;

        /// <summary>
        /// The granularity the budget is handed out in. Smaller means finer-grained overlap between
        /// the walk and the workers, at the cost of more locking.
        /// </summary>
        size_t chunkSize = 2ull * 1024 * 1024;

        /// <summary>
        /// Token that cancels both the archive walk and any in-progress layer decompression.
        /// </summary>
        std::stop_token stopToken{};
    };

    /// <summary>
    /// Accumulates OCI image manifest/config details from an archive's entries, fed one at a time.
    /// </summary>
    /// <remarks>
    /// It is designed to parse an image in a single unordered pass, so has to allocate more than strictly necessary -
    /// e.g. by storing the contents of every JSON file encountered in case one of them turns out to be the "Config".
    /// But these files are usually on the order of kilobytes in size so this is an acceptable compromise.
    ///
    /// Parsing is CPU-bound on gzip decompression of the layer blobs, and each layer's diff is independent
    /// of every other, so those are handed off to a thread pool. The archive is forward-only - it may
    /// eventually be `docker save` piped to stdin - so the walk cannot leave a layer for a worker to
    /// come back to: it reads ahead into pooled chunks, which a worker drains and returns while the
    /// walk is still filling them. ChunkPool is what bounds that read-ahead.
    /// </remarks>
    class ImageParser
    {
      public:
        /// <summary>
        /// Constructs a new instance of the ImageParser class.
        /// </summary>
        /// <param name="options">Threading and memory configuration.</param>
        explicit ImageParser(ImageParserOptions options = {});

        ~ImageParser();

        ImageParser(const ImageParser&) = delete;
        ImageParser& operator=(const ImageParser&) = delete;

        /// <summary>
        /// Processes an individual archive entry, and store it in the parser's internal state for a later Build() call.
        /// </summary>
        /// <remarks>
        /// For layer blobs this returns once the compressed bytes have been pumped across to a worker -
        /// the decompression itself may still be in flight. Blocks while the read-ahead budget is
        /// exhausted, which is the backpressure keeping memory bounded.
        /// </remarks>
        /// <param name="entry">The archive entry.</param>
        void ProcessEntry(ArchiveEntry& entry);

        /// <summary>
        /// Waits for any outstanding layer parsing to finish, then assembles a vector of image layers.
        /// Call once all archive entries have been handed to ProcessEntry().
        /// </summary>
        /// <remarks>
        /// Blocking, and potentially for a long time - do not call this on a UI thread. Rethrows the
        /// first exception thrown by a worker, if there was one.
        /// </remarks>
        /// <returns>A vector of image layers.</returns>
        std::vector<ImageLayer> Build();

        /// <summary>
        /// How long the archive walk spent blocked waiting for read-ahead memory. Zero means the
        /// budget never got in the way; a large value means raising it would buy throughput.
        /// </summary>
        std::chrono::nanoseconds ReadAheadStallTime() const
        {
            return m_chunkPool ? m_chunkPool->StalledFor() : std::chrono::nanoseconds{0};
        }

      private:
        /// <summary>
        /// Handles an entry under blobs/sha256/, which is either a small JSON document or a layer.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        void ProcessBlob(ArchiveEntry& entry);

        /// <summary>
        /// Pumps a layer's compressed bytes into a pipe that a worker thread decompresses as they arrive.
        /// </summary>
        /// <param name="entry">The archive entry positioned at the layer.</param>
        /// <param name="archivePath">The layer's path within the archive.</param>
        /// <param name="prefix">Bytes already read from the entry while sniffing it.</param>
        void StreamLayerToWorker(ArchiveEntry& entry, std::string archivePath, std::span<const std::byte> prefix);

        /// <summary>
        /// Decompresses a layer on the calling thread, reading straight from the archive.
        /// </summary>
        /// <param name="entry">The archive entry positioned at the layer.</param>
        /// <param name="archivePath">The layer's path within the archive.</param>
        /// <param name="prefix">Bytes already read from the entry while sniffing it.</param>
        void ParseLayerInline(ArchiveEntry& entry, std::string archivePath, std::span<const std::byte> prefix);

        /// <summary>
        /// Records a parsed layer diff. Safe to call from a worker thread.
        /// </summary>
        void StoreLayer(std::string archivePath, FilesystemChangeTreeNodeRef tree);

        /// <summary>
        /// Records a worker's exception if it is the first one, to be rethrown from Build().
        /// </summary>
        void RecordWorkerException(std::exception_ptr exception);

        /// <summary>
        /// Blocks until fewer than workerCount layers are in flight.
        /// </summary>
        /// <remarks>
        /// This is what makes the chunk pool deadlock-free: every layer holding chunks has a thread
        /// actively draining it, so a walk blocked waiting for memory is always waiting on a worker
        /// that is about to hand some back.
        /// </remarks>
        void AcquireLayerSlot();
        void ReleaseLayerSlot();

        ImageParserOptions m_options;

        // Both engaged only when running with more than one worker.
        std::optional<ChunkPool> m_chunkPool;
        std::optional<ThreadPool> m_workerPool;

        std::mutex m_slotMutex;
        std::condition_variable_any m_slotFreed;
        size_t m_layersInFlight{0};

        std::mutex m_resultsMutex;
        std::exception_ptr m_firstWorkerException;

        std::optional<std::string> m_configPath;
        std::vector<std::string> m_layerPaths;

        // Raw, not-yet-parsed JSON text keyed by digest hashes
        std::unordered_map<std::string, std::string> m_jsonBlobsByDigest;

        // Written from worker threads - guarded by m_resultsMutex until Build() has joined them.
        std::unordered_map<std::string, FilesystemChangeTreeNodeRef> m_filesystemsByLayerDigest;
    };

} // namespace Flense::Core
