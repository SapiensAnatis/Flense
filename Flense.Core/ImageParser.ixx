module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:ImageParser;

import :ArchiveReader;
import :FileByteStream;
import :Internal.BufferPool;
import :Internal.Channel;
import :Internal.ChannelByteStream;
import :Filesystem;
import :Image;
import :Internal.Mutex;
import std;

namespace Flense::Core
{
    namespace
    {
        /// <summary>
        /// Sleeps for the given duration, waking early if stopToken is stopped.
        /// </summary>
        void SleepFor(const std::chrono::milliseconds duration, const std::stop_token stopToken)
        {
            std::mutex mutex;
            std::condition_variable_any cv;
            std::unique_lock lock(mutex);

            // Nothing ever notifies cv, so this only returns on timeout or when stopToken is stopped.
            cv.wait_for(lock, stopToken, duration, [] { return false; });
        }
    } // namespace
} // namespace Flense::Core

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
        ImageParser() = default;
        ~ImageParser() = default;

        ImageParser(const ImageParser&) = delete;
        ImageParser& operator=(const ImageParser&) = delete;
        ImageParser(ImageParser&&) = delete;
        ImageParser& operator=(ImageParser&&) = delete;

        /// <summary>
        /// Processes an individual archive entry, and store it in the parser's internal state for a later Build() call.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        /// <param name="stopToken">The token to check for cancellation. Parsing a layer blob can take a long time, so
        /// it is checked once per entry within the layer's nested archive rather than only between top-level entries.
        /// If cancellation is requested part-way through, the parser is left holding partial state and Build() must
        /// not be called.</param>
        /// <exception cref="OperationCancelledError">stopToken was triggered before the entry was read in
        /// full.</exception>
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
        /// <returns>An ImageDetails struct.</returns>
        /// <exception cref="OperationCancelledError">stopToken was triggered while waiting for outstanding
        /// work.</exception>
        [[nodiscard]] Image Build(std::stop_token stopToken = {});

        [[nodiscard]] std::uint64_t BytesProcessed() const
        {
            return m_bytesProcessed.load(std::memory_order_relaxed);
        }

      private:
        struct Worker;

        using UpstreamStopRegistration = std::stop_callback<std::function<void()>>;

        struct Worker
        {
            Worker(std::jthread&& thread, std::future<void>&& result)
                : thread(std::move(thread)), result(std::move(result))
            {
            }

            std::jthread thread;
            std::future<void> result;
            std::optional<UpstreamStopRegistration> propagateStop;
        };

        /// <summary>
        /// Hands a nested layer tar's bytes off to a worker thread for parsing. Drains the entry into buffer
        /// chunks on the calling thread first - this must complete, and so this call must return, before the
        /// owning ArchiveReader's Next() is called again.
        /// </summary>
        /// <param name="entry">The archive entry, whose sniffed prefix has already been consumed.</param>
        /// <param name="sniffedPrefix">The bytes already read from entry while sniffing for JSON.</param>
        /// <param name="stopToken">The token to check for cancellation.</param>
        void DispatchLayerWorker(ArchiveEntry& entry, std::span<const std::byte> sniffedPrefix,
                                 std::stop_token stopToken);

        /// <summary>
        /// Waits for every dispatched worker to finish. If one threw, every worker is still joined (they
        /// capture `this`, so none can be left running) before that exception is rethrown to the caller;
        /// exceptions from any other workers are discarded. Throws OperationCancelledError if stopToken is triggered
        /// while waiting, again only once every worker has been joined.
        /// </summary>
        void JoinWorkers(std::stop_token stopToken);

        /// <summary>
        /// Implementation of ProcessEntry. ProcessEntry calls this with added error handling.
        /// </summary>
        /// <param name="entry">The entry to process.</param>
        /// <param name="stopToken">The token to check for cancellation.</param>
        void ProcessEntryInternal(ArchiveEntry& entry, std::stop_token stopToken);

        static constexpr std::size_t WorkerMaxMemoryUsage = std::size_t{128} * 1024 * 1024;
        static constexpr std::size_t WorkerBufferSize = std::size_t{256} * 1024;
        static constexpr std::size_t WorkerBufferCount = WorkerMaxMemoryUsage / WorkerBufferSize;

        BufferPool m_bufferPool{WorkerBufferSize, WorkerBufferCount};

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

        // Must be destroyed first to ensure any pending threads are stopped before they use-after-free
        std::list<Worker> m_workers;
    };

    using ArchiveProgressCallback = std::function<void(std::uint64_t bytesProcessed, std::uint64_t totalBytes)>;

    /// <summary>
    /// Parses an image archive file on disk into an Image. Blocks until parsing is complete, so it should be called
    /// from a background thread.
    /// </summary>
    /// <param name="path">The path to the image archive.</param>
    /// <param name="onProgress">Optional callback invoked periodically, from a background thread, with cumulative
    /// byte totals.</param>
    /// <param name="stopToken">The token to check for cancellation.</param>
    /// <returns>The parsed Image.</returns>
    /// <exception cref="std::filesystem::filesystem_error">The file could not be opened or read.</exception>
    /// <exception cref="ImageFormatError">The archive is not a valid image.</exception>
    /// <exception cref="OperationCancelledError">stopToken was triggered before parsing finished.</exception>
    [[nodiscard]] Image ParseImageArchive(const std::filesystem::path& path,
                                          const ArchiveProgressCallback& onProgress = {},
                                          const std::stop_token stopToken = {})
    {
        FileByteStream stream{path};
        const std::uint64_t totalBytes = stream.Size();

        auto reader = ArchiveReader::CreateFromStream(stream);
        ImageParser parser;

        // Declared after parser so that it is stopped before parser is destroyed.
        std::jthread progressReporter;
        if (onProgress)
        {
            progressReporter = std::jthread([&parser, &onProgress, totalBytes](const std::stop_token reporterStop) {
                constexpr std::chrono::milliseconds ProgressInterval{250};

                while (!reporterStop.stop_requested())
                {
                    onProgress(parser.BytesProcessed(), totalBytes);
                    SleepFor(ProgressInterval, reporterStop);
                }
            });
        }

        while (auto entry = reader.Next(stopToken))
        {
            parser.ProcessEntry(*entry, stopToken);
        }

        return parser.Build(stopToken);
    }

} // namespace Flense::Core
