#pragma once

#include "ByteBudget.h"
#include "FilesystemTree.h"
#include "ThreadPool.h"

#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
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
        /// happens inline on the thread calling ProcessEntry, with no pool created at all.
        /// </summary>
        size_t workerCount = 0;

        /// <summary>
        /// The ceiling on how many bytes of not-yet-decompressed layer blobs may be held in memory at
        /// once. ProcessEntry blocks when handing over a blob would exceed it, so this bounds the
        /// parser's peak memory rather than letting it scale with the image size.
        /// </summary>
        uint64_t inFlightByteBudget = 512ull * 1024 * 1024;

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
    /// of every other, so those are handed off to a thread pool. The archive itself is a forward-only
    /// stream - it may eventually be `docker save` piped to stdin - which means a blob cannot be left
    /// for a worker to read lazily: its compressed bytes are copied out in full, on the calling thread,
    /// before the walk can advance to the next entry. See ImageParserOptions::inFlightByteBudget.
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
        /// For layer blobs this returns once the compressed bytes have been copied out of the entry -
        /// the decompression itself may still be in flight on a worker thread. Blocks while the
        /// in-flight byte budget is exhausted.
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

      private:
        /// <summary>
        /// Decompresses a layer blob and records its filesystem diff, then returns its bytes to the
        /// budget. Runs on a worker thread when there is a pool, inline otherwise.
        /// </summary>
        /// <param name="archivePath">The blob's path within the outer archive.</param>
        /// <param name="compressedBytes">The blob's compressed bytes.</param>
        void ParseLayerBlob(std::string archivePath, std::vector<std::byte> compressedBytes);

        ImageParserOptions m_options;
        ByteBudget m_budget;

        // Engaged only when running with more than one worker.
        std::optional<ThreadPool> m_pool;

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
