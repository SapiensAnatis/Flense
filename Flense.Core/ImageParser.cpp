module Flense.Core;

import :ArchiveReader;
import :BufferPool;
import :Channel;
import :ChannelByteStream;
import :FilesystemParsing;
import :Filesystem;
import :Image;
import :ImageParser;
import :Mutex;

import nlohmann_json;
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
        (void)stopToken;

        const std::string_view pathname = entry.Pathname();

        if (pathname == "manifest.json")
        {
            const std::uint64_t entrySize = entry.Size();
            ManifestDetails manifest = ParseManifestJson(entry);

            m_bytesProcessed.fetch_add(entrySize, std::memory_order_relaxed);

            m_configPath = std::move(manifest.configPath);
            m_repoTag = std::move(manifest.repoTag);
            m_layerPaths = std::move(manifest.layerPaths);
            return;
        }

        if (!pathname.starts_with(BlobPrefix))
        {
            return;
        }

        const std::uint64_t entrySize = entry.Size();

        // Sniff a small, bounded prefix first - large layer blobs must never be buffered in
        // full just to find out they're not JSON.
        std::array<char, 64> sniffBuffer{};
        const std::size_t sniffLength = std::min(sniffBuffer.size(), static_cast<std::size_t>(entrySize));
        const std::span<char> sniffSpan = std::span{sniffBuffer}.first(sniffLength);

        const std::size_t sniffed = entry.ReadInto(std::as_writable_bytes(sniffSpan));

        if (LooksLikeJsonObject(std::string_view{sniffBuffer.data(), sniffed}))
        {
            // Confirmed JSON (manifest/config/etc. - always small) - now safe to buffer in full and parse inline.
            std::string contents(sniffBuffer.data(), sniffed);
            contents.resize(entrySize);

            const std::span<char> remainingSpan = std::span{contents}.subspan(sniffed);
            const std::size_t read = entry.ReadInto(std::as_writable_bytes(remainingSpan));
            contents.resize(sniffed + read);

            m_bytesProcessed.fetch_add(sniffed + read, std::memory_order_relaxed);

            m_jsonBlobsByDigest.emplace(std::string{pathname}, std::move(contents));
            return;
        }

        // This is a tar file containing layer diffs - hand it off to a worker thread so the (CPU-heavy)
        // parsing can proceed while the reader moves on to the next top-level entry.
        DispatchLayerWorker(std::string{pathname}, entry, entrySize,
                            std::as_bytes(std::span{sniffBuffer}.first(sniffed)));
    }

    void ImageParser::DispatchLayerWorker(std::string archivePath, ArchiveEntry& entry, const std::uint64_t entrySize,
                                          const std::span<const std::byte> sniffedPrefix)
    {
        auto channel = std::make_shared<Channel<BufferChunk>>();

        std::promise<void> promise;
        std::future<void> future = promise.get_future();

        std::jthread thread([this, channel, archivePath = std::move(archivePath), entrySize,
                             promise = std::move(promise)](std::stop_token workerStopToken) mutable {
            try
            {
                ChannelByteStream stream(channel.get(), entrySize);
                auto reader = ArchiveReader::CreateFromStream(stream);
                FilesystemChangeTreeNodeRef filesystem = ParseLayerFilesystem(&reader, workerStopToken);

                {
                    const MutexLocker locker(&m_filesystemMutex);
                    m_filesystemsByLayerDigest.emplace(std::move(archivePath), std::move(filesystem));
                }

                m_bytesProcessed.fetch_add(entrySize, std::memory_order_relaxed);

                promise.set_value();
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        });

        m_workers.push_back(Worker{.thread = std::move(thread), .result = std::move(future)});

        bool exhausted = false;

        {
            RentedBuffer buffer = RentedBuffer::From(&m_bufferPool);
            const std::span<std::byte> bufferSpan = buffer.Buffer();

            std::ranges::copy(sniffedPrefix, bufferSpan.begin());

            const std::size_t additionalRead = entry.ReadInto(bufferSpan.subspan(sniffedPrefix.size()));
            const std::size_t length = sniffedPrefix.size() + additionalRead;

            exhausted = length < bufferSpan.size();

            channel->Push(BufferChunk{.buffer = std::move(buffer), .length = length});
        }

        while (!exhausted)
        {
            RentedBuffer buffer = RentedBuffer::From(&m_bufferPool);
            const std::span<std::byte> bufferSpan = buffer.Buffer();
            const std::size_t length = entry.ReadInto(bufferSpan);

            exhausted = length < bufferSpan.size();

            channel->Push(BufferChunk{.buffer = std::move(buffer), .length = length});
        }

        channel->Close();
    }

    void ImageParser::ReportProgressPeriodically(const ProgressCallback& onProgress,
                                                 const std::stop_token stopToken) const
    {
        std::mutex mutex;
        std::condition_variable_any cv;
        std::unique_lock lock(mutex);

        while (true)
        {
            // Interruptible sleep
            cv.wait_for(lock, stopToken, ProgressInterval, [] { return false; });

            if (stopToken.stop_requested())
            {
                break;
            }

            onProgress(m_bytesProcessed.load(std::memory_order_relaxed));
        }
    }

    void ImageParser::JoinWorkers()
    {
        for (std::size_t i = 0; i < m_workers.size(); ++i)
        {
            try
            {
                m_workers.at(i).result.get();
            }
            catch (...)
            {
                // Every worker captured `this` in its lambda, so none can be left running once we let this
                // exception escape. Clearing the vector destroys each remaining jthread, which requests
                // cancellation and joins it on the way out - no need to do that by hand here.
                m_workers.clear();
                throw;
            }
        }

        m_workers.clear();
    }

    Image ImageParser::Build(const ProgressCallback& onProgress)
    {
        {
            std::jthread progressReporter;

            if (onProgress)
            {
                progressReporter = std::jthread([this, &onProgress](const std::stop_token stopToken) {
                    ReportProgressPeriodically(onProgress, stopToken);
                });
            }

            JoinWorkers();
        } // progressReporter is stopped and joined here, before we go on to assemble the Image below.

        if (!m_configPath)
        {
            return Image{};
        }

        std::vector<ImageLayer> layers;

        const auto configIt = m_jsonBlobsByDigest.find(*m_configPath);
        if (configIt != m_jsonBlobsByDigest.end())
        {
            const nlohmann::json config = nlohmann::json::parse(configIt->second);
            const nlohmann::json& historyArray = config.at("history");

            const MutexLocker locker(&m_filesystemMutex);

            std::size_t layerNum = 0;
            FilesystemChangeTreeNodeRef currentFsSnapshot{nullptr};

            for (const auto& historyObj : historyArray)
            {
                if (auto it = historyObj.find("empty_layer"); it != historyObj.end() && it->get<bool>())
                {
                    continue;
                }

                const std::string_view command = historyObj.at("created_by").get_ref<const std::string&>();

                // TODO: Make this more destructive so the filesystem tree can be moved?
                // Though, does it really matter if it's just a shared pointer?
                const std::string& layerPath = m_layerPaths.at(layerNum);
                const FilesystemChangeTreeNodeRef& diff = m_filesystemsByLayerDigest.at(layerPath);

                if (currentFsSnapshot)
                {
                    currentFsSnapshot = ApplyFilesystemChanges(currentFsSnapshot, diff);
                }
                else
                {
                    currentFsSnapshot = diff;
                }

                layers.emplace_back(CollapseWhitespace(command), currentFsSnapshot);

                layerNum += 1;
            }
        }

        return Image{
            .repoTag = m_repoTag,
            .layers = std::move(layers),
        };
    }
} // namespace Flense::Core
