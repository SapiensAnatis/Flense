module;

#include <nlohmann/json.hpp>

module Flense.Core:ImageParser.Impl;

import :ArchiveReader;
import :BufferPool;
import :Channel;
import :ChannelByteStream;
import :FilesystemParsing;
import :Filesystem;
import :Image;
import :ImageParser;
import :Mutex;

import std;

namespace Flense::Core
{
    namespace
    {
        /// <summary>
        /// Represents details parsed from manifest.json.
        /// </summary>
        struct ManifestDetails
        {
            std::string configPath;
            std::optional<std::string> repoTag;
            std::vector<std::string> layerPaths;
        };

        constexpr std::string_view BlobPrefix = "blobs/sha256/";
        constexpr std::chrono::seconds ProgressInterval{2};

        /// <summary>
        /// Checks if a character is a space or a tab. Matches fewer characters than isspace and is locale-independent.
        /// </summary>
        /// <param name="c">The character to test.</param>
        /// <returns>Whether c is a space or a tab.</returns>
        [[nodiscard]] bool IsSpaceOrTab(const char c)
        {
            return c == ' ' || c == '\t';
        }

        /// <summary>
        /// Tests whether a blob could plausibly be a JSON file with an object as the root member.
        /// </summary>
        /// <param name="prefix">The first few bytes of the blob.</param>
        /// <returns>A value indicating whether the blob looks like a JSON object.</returns>
        bool LooksLikeJsonObject(const std::string_view prefix)
        {
            const auto it = std::ranges::find_if(prefix, [](char c) { return !IsSpaceOrTab(c); });
            return it != prefix.end() && *it == '{';
        }

        /// <summary>
        /// Parses a manifest.json file into a ManifestDetails struct.
        /// </summary>
        /// <param name="entry">The archive entry whose pathname has been identified as manifest.json.</param>
        /// <returns>A ManifestDetails struct.</returns>
        ManifestDetails ParseManifestJson(const ArchiveEntry& entry)
        {
            std::string contents;
            contents.resize(entry.Size());

            const std::span<char> contentsSpan = std::span{contents};
            const std::size_t bytesRead = entry.ReadInto(std::as_writable_bytes(contentsSpan));
            contents.resize(bytesRead);

            const nlohmann::json manifests = nlohmann::json::parse(contents);

            // TODO: support archives containing multiple images (manifest.json is an array of them) -
            // for now we just take the first one.
            const nlohmann::json& manifest = manifests.at(0);

            const std::string configPath = manifest.at("Config").get<std::string>();
            const std::optional<std::string> repoTag = [&manifest]() -> std::optional<std::string> {
                const auto& tags = manifest.at("RepoTags");

                // If an image is untagged, "RepoTags" will be null.
                if (tags.is_array() && !tags.empty())
                {
                    return tags.at(0).get<std::string>();
                }

                return std::nullopt;
            }();

            std::vector<std::string> layerPaths;
            for (const auto& layerPath : manifest.at("Layers"))
            {
                layerPaths.push_back(layerPath.get<std::string>());
            }

            return ManifestDetails{
                .configPath = configPath,
                .repoTag = repoTag,
                .layerPaths = std::move(layerPaths),
            };
        }

        /// <summary>
        /// Collapses runs of whitespace and/or tab characters into a single whitespace character.
        /// </summary>
        /// <param name="string">The input string.</param>
        /// <returns>A new string with whitespace collased.</returns>
        std::string CollapseWhitespace(const std::string_view string)
        {
            std::string out;
            out.reserve(string.size());

            bool previousIsSpace = false;

            for (const char c : string)
            {
                const bool isSpace = IsSpaceOrTab(c);
                const bool keep = !isSpace || !previousIsSpace;

                if (keep)
                {
                    out.push_back(isSpace ? ' ' : c);
                }

                previousIsSpace = isSpace;
            }

            return out;
        }
    } // namespace

    void ImageParser::ProcessEntry(ArchiveEntry& entry, std::stop_token stopToken)
    {
        try
        {
            ProcessEntryInternal(entry, stopToken);
        }
        catch (ImageReadError& error)
        {
            error.SetEntryPath(std::string{entry.Pathname()});
            throw;
        }
    }

    void ImageParser::ProcessEntryInternal(ArchiveEntry& entry, std::stop_token stopToken)
    {
        if (stopToken.stop_requested())
        {
            throw OperationCancelledError{};
        }

        // Ignore folders/sockets/symlinks/etc.
        if (entry.FileKind() != FileKind::File)
        {
            return;
        }

        const std::string_view pathname = entry.Pathname();

        static constexpr std::size_t MaxJsonSize = 10'000'000; // 10 MB

        if (pathname == "manifest.json")
        {
            if (entry.Size() > MaxJsonSize)
            {
                // Defend against maliciously crafted images with huge manifests
                throw ImageFormatError{ImageFormatErrorKind::MalformedMetadata, "manifest.json is too large"};
            }

            const std::uint64_t entrySize = entry.Size();
            ManifestDetails manifest;

            try
            {
                manifest = ParseManifestJson(entry);
            }
            catch (const nlohmann::json::exception& e)
            {
                throw ImageFormatError{ImageFormatErrorKind::MalformedMetadata, e.what()};
            }

            m_bytesProcessed.fetch_add(entrySize, std::memory_order_relaxed);

            m_configPath = std::move(manifest.configPath);
            m_repoTag = std::move(manifest.repoTag);
            m_layerPaths = std::move(manifest.layerPaths);
            return;
        }

        if (!pathname.starts_with(BlobPrefix))
        {
            // Unrelated, random file
            return;
        }

        // Sniff a small, bounded prefix first - large layer blobs must never be buffered in
        // full just to find out they're not JSON.
        std::array<char, 64> sniffBuffer{};
        const std::size_t sniffLength = std::min(sniffBuffer.size(), static_cast<std::size_t>(entry.Size()));
        const std::span<char> sniffSpan = std::span{sniffBuffer}.first(sniffLength);

        const std::size_t sniffed = entry.ReadInto(std::as_writable_bytes(sniffSpan));

        if (LooksLikeJsonObject(std::string_view{sniffBuffer.data(), sniffed}))
        {
            std::string contents(sniffBuffer.data(), sniffed);

            // Cap the amount of JSON we buffer into memory. If the entry is larger than this, it will be truncated and
            // the JSON parser will throw. This is preferable to skipping the entry as it will fail loudly and is easier
            // to debug.
            contents.resize(static_cast<std::size_t>(std::min<std::uint64_t>(entry.Size(), MaxJsonSize)));

            const std::span<char> remainingSpan = std::span{contents}.subspan(sniffed);
            const std::size_t read = entry.ReadInto(std::as_writable_bytes(remainingSpan));
            contents.resize(sniffed + read);

            m_jsonBlobsByDigest.emplace(std::string{pathname}, std::move(contents));

            m_bytesProcessed.fetch_add(sniffed + read, std::memory_order_relaxed);

            return;
        }

        // This is a tar file containing layer diffs - hand it off to a worker thread so the (CPU-heavy)
        // parsing can proceed while the reader moves on to the next top-level entry.
        DispatchLayerWorker(entry, std::as_bytes(std::span{sniffBuffer}.first(sniffed)), stopToken);
    }

    void ImageParser::DispatchLayerWorker(ArchiveEntry& entry, const std::span<const std::byte> sniffedPrefix,
                                          const std::stop_token stopToken)
    {
        auto [sender, receiver] = Channel<BufferChunk>::Create();

        std::promise<void> promise;
        std::future<void> future = promise.get_future();

        std::jthread thread([this, receiver = std::move(receiver), archivePath = std::string(entry.Pathname()),
                             entrySize = entry.Size(),
                             promise = std::move(promise)](std::stop_token workerStopToken) mutable {
            try
            {
                ChannelByteStream stream(std::move(receiver), entrySize);
                auto reader = ArchiveReader::CreateFromStream(stream);
                FilesystemChangeTreeNodeRef filesystem = ParseLayerFilesystem(&reader, workerStopToken);

                {
                    const MutexLocker locker(&m_filesystemMutex);
                    m_filesystemsByLayerDigest.emplace(archivePath, std::move(filesystem));
                }

                m_bytesProcessed.fetch_add(entrySize, std::memory_order_relaxed);

                promise.set_value();
            }
            catch (ImageReadError& error)
            {
                error.SetEntryPath(archivePath);
                promise.set_exception(std::current_exception());
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        });

        Worker& worker = m_workers.emplace_back(std::move(thread), std::move(future));

        // Store stop callback in worker so that it can outlive the producer thread
        worker.propagateStop.emplace(stopToken, [&worker] { worker.thread.request_stop(); });

        // If the producer thread quits, through cancellation or an error, the sender will be destroyed and the channel
        // will be automatically closed in ~Sender.

        if (stopToken.stop_requested())
        {
            throw OperationCancelledError{};
        }

        bool exhausted = false;

        {
            RentedBuffer buffer = RentedBuffer::From(&m_bufferPool);
            const std::span<std::byte> bufferSpan = buffer.Buffer();

            std::ranges::copy(sniffedPrefix, bufferSpan.begin());

            const std::size_t additionalRead = entry.ReadInto(bufferSpan.subspan(sniffedPrefix.size()));
            const std::size_t length = sniffedPrefix.size() + additionalRead;

            exhausted = length < bufferSpan.size();

            if (!sender.Push(BufferChunk{.buffer = std::move(buffer), .length = length}))
            {
                return;
            }
        }

        while (!exhausted)
        {
            if (stopToken.stop_requested())
            {
                throw OperationCancelledError{};
            }

            RentedBuffer buffer = RentedBuffer::From(&m_bufferPool);
            const std::span<std::byte> bufferSpan = buffer.Buffer();
            const std::size_t length = entry.ReadInto(bufferSpan);

            exhausted = length < bufferSpan.size();

            if (!sender.Push(BufferChunk{.buffer = std::move(buffer), .length = length}))
            {
                return;
            }
        }
    }

    void ImageParser::ReportProgressPeriodically(const ProgressCallback& onProgress,
                                                 const std::stop_token stopToken) const
    {
        std::mutex mutex;
        std::condition_variable_any cv;
        std::unique_lock lock(mutex);

        while (!stopToken.stop_requested())
        {
            // Interruptible sleep
            onProgress(m_bytesProcessed.load(std::memory_order_relaxed));

            cv.wait_for(lock, stopToken, ProgressInterval, [] { return false; });
        }
    }

    void ImageParser::JoinWorkers(const std::stop_token stopToken)
    {
        std::list<Worker> ownedWorkers = std::exchange(m_workers, {});

        for (Worker& worker : ownedWorkers)
        {
            if (stopToken.stop_requested())
            {
                throw OperationCancelledError{};
            }

            worker.result.get();
        }
    }

    Image ImageParser::Build(const ProgressCallback& onProgress, const std::stop_token stopToken)
    {
        {
            std::jthread progressReporter;

            if (onProgress)
            {
                progressReporter = std::jthread([this, &onProgress](const std::stop_token stopToken) {
                    ReportProgressPeriodically(onProgress, stopToken);
                });
            }

            JoinWorkers(stopToken);
        } // Join progress thread; we've now read all of the bytes of the file and just have to do some bookkeeping

        if (!m_configPath)
        {
            throw ImageFormatError{ImageFormatErrorKind::NoManifest, "No manifest.json entry"};
        }

        const auto configIt = m_jsonBlobsByDigest.find(*m_configPath);
        if (configIt == m_jsonBlobsByDigest.end())
        {
            throw ImageFormatError{ImageFormatErrorKind::MissingConfiguration, "No entry at the manifest's Config path",
                                   *m_configPath};
        }

        // The commands are views into config, so it has to outlive the loop that consumes them below.
        nlohmann::json config;
        std::vector<std::string_view> commands;

        try
        {
            config = nlohmann::json::parse(configIt->second);

            for (const auto& historyObj : config.at("history"))
            {
                if (auto it = historyObj.find("empty_layer"); it != historyObj.end() && it->get<bool>())
                {
                    continue;
                }

                commands.emplace_back(historyObj.at("created_by").get_ref<const std::string&>());
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            throw ImageFormatError{ImageFormatErrorKind::MalformedMetadata, e.what(), *m_configPath};
        }

        if (commands.size() > m_layerPaths.size())
        {
            throw ImageFormatError{ImageFormatErrorKind::MissingLayers,
                                   "Manifest lists fewer layers than the configuration's history"};
        }

        std::vector<ImageLayer> layers;
        layers.reserve(commands.size());

        const MutexLocker locker(&m_filesystemMutex);

        FilesystemChangeTreeNodeRef currentFsSnapshot{nullptr};

        for (std::size_t layerNum = 0; layerNum < commands.size(); ++layerNum)
        {
            const std::string& layerPath = m_layerPaths.at(layerNum);

            const auto diffIt = m_filesystemsByLayerDigest.find(layerPath);
            if (diffIt == m_filesystemsByLayerDigest.end())
            {
                throw ImageFormatError{ImageFormatErrorKind::MissingLayers, "No entry at the manifest's layer path",
                                       layerPath};
            }

            // TODO: Make this more destructive so the filesystem tree can be moved?
            // Though, does it really matter if it's just a shared pointer?
            const FilesystemChangeTreeNodeRef& diff = diffIt->second;

            if (currentFsSnapshot)
            {
                currentFsSnapshot = ApplyFilesystemChanges(currentFsSnapshot, diff);
            }
            else
            {
                currentFsSnapshot = diff;
            }

            layers.emplace_back(CollapseWhitespace(commands.at(layerNum)), currentFsSnapshot);
        }

        return Image{
            .repoTag = m_repoTag,
            .layers = std::move(layers),
        };
    }
} // namespace Flense::Core
