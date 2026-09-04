module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:ImageParser;

import :ArchiveReader;
import :BufferPool;
import :Filesystem;
import :Image;
import :Mutex;
import std;

export namespace Flense::Core
{

    /// <summary>
    /// Accumulates OCI image manifest/config details from an archive's entries, fed one at a time.
    /// </summary>
    /// <remarks>
    /// It is designed to parse an image in a single unordered pass, so has to allocate more than strictly necessary -
    /// e.g. by storing the contents of every JSON file encountered in case one of them turns out to be the "Config".
    /// But these files are usually on the order of kilobytes in size so this is an acceptable compromise.
    /// </remarks>
    class ImageParser
    {
      public:
        using ProgressCallback = std::function<void(std::uint64_t bytesProcessed)>;

        /// <summary>
        /// Processes an individual archive entry, and store it in the parser's internal state for a later Build() call.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        /// <param name="stopToken">The token to check for cancellation. Parsing a layer blob can take a long time, so
        /// it is checked once per entry within the layer's nested archive rather than only between top-level entries.
        /// If cancellation is requested part-way through, the parser is left holding partial state and Build() must
        /// not be called.</param>
        /// <remarks>
        /// Entries that turn out to be nested layer tars are handed off to a worker thread for parsing and this
        /// call returns as soon as the entry's bytes have been read off the archive, without waiting for that
        /// parsing to finish - so this call must not be followed by another call on the owning ArchiveReader
        /// until it returns, but the reader itself is free to move on to the next entry immediately after.
        /// </remarks>
        void ProcessEntry(ArchiveEntry& entry, std::stop_token stopToken);

        /// <summary>
        /// Waits for all outstanding layer-parsing work dispatched by ProcessEntry() to finish, then assembles
        /// an Image from the accumulated state.
        /// </summary>
        /// <param name="onProgress">Optional callback invoked periodically, from a background thread, with
        /// cumulative byte totals while waiting for outstanding work to finish.</param>
        /// <returns>An ImageDetails struct.</returns>
        [[nodiscard]] Image Build(const ProgressCallback& onProgress = {});

        [[nodiscard]] std::uint64_t BytesProcessed() const
        {
            return m_bytesProcessed.load(std::memory_order_relaxed);
        }

      private:
        struct Worker
        {
            std::jthread thread;
            std::future<void> result;
        };

        /// <summary>
        /// Hands a nested layer tar's bytes off to a worker thread for parsing. Drains the entry into buffer
        /// chunks on the calling thread first - this must complete, and so this call must return, before the
        /// owning ArchiveReader's Next() is called again.
        /// </summary>
        /// <param name="archivePath">The entry's path, to key the resulting filesystem tree by.</param>
        /// <param name="entry">The archive entry, whose sniffed prefix has already been consumed.</param>
        /// <param name="entrySize">The entry's total size, as reported by the archive header.</param>
        /// <param name="sniffedPrefix">The bytes already read from entry while sniffing for JSON.</param>
        void DispatchLayerWorker(std::string archivePath, ArchiveEntry& entry, std::uint64_t entrySize,
                                 std::span<const std::byte> sniffedPrefix);

        /// <summary>
        /// Calls onProgress with the current byte totals every ProgressInterval, until stopToken is stopped.
        /// </summary>
        void ReportProgressPeriodically(const ProgressCallback& onProgress, std::stop_token stopToken) const;

        /// <summary>
        /// Waits for every dispatched worker to finish. If one threw, every worker is still joined (they
        /// capture `this`, so none can be left running) before that exception is rethrown to the caller;
        /// exceptions from any other workers are discarded.
        /// </summary>
        void JoinWorkers();

        static constexpr std::size_t WorkerMaxMemoryUsage = std::size_t{128} * 1024 * 1024;
        static constexpr std::size_t WorkerBufferSize = std::size_t{256} * 1024;
        static constexpr std::size_t WorkerBufferCount = WorkerMaxMemoryUsage / WorkerBufferSize;

        BufferPool m_bufferPool{WorkerBufferSize, WorkerBufferCount};
        std::vector<Worker> m_workers;

        std::optional<std::string> m_configPath;
        std::optional<std::string> m_repoTag;
        std::vector<std::string> m_layerPaths;

        // Raw, not-yet-parsed JSON text keyed by digest hashes. Only ever touched from ProcessEntry's caller
        // (the reader thread), never from a worker, so it needs no locking.
        std::unordered_map<std::string, std::string> m_jsonBlobsByDigest;

        // Written concurrently by worker threads dispatched from DispatchLayerWorker, so needs locking.
        Mutex m_filesystemMutex;
        std::unordered_map<std::string, FilesystemChangeTreeNodeRef> m_filesystemsByLayerDigest
            GUARDED_BY(m_filesystemMutex);

        std::atomic<std::uint64_t> m_bytesProcessed{0};
    };

} // namespace Flense::Core
