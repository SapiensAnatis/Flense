#include "pch.h"

#include "ArchiveReader.h"
#include "ChunkPool.h"
#include "ChunkQueueByteStream.h"
#include "EntryByteStream.h"
#include "FilesystemParsing.h"
#include "FilesystemTree.h"
#include "ImageLayer.h"
#include "ImageParser.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <exception>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

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
            std::vector<std::string> layerPaths;
        };

        constexpr std::string_view BlobPrefix = "blobs/sha256/";
        constexpr size_t SniffLength = 64;

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
            const size_t bytesRead = entry.ReadInto(std::as_writable_bytes(contentsSpan));
            contents.resize(bytesRead);

            const nlohmann::json manifests = nlohmann::json::parse(contents);

            // TODO: support archives containing multiple images (manifest.json is an array of them) -
            // for now we just take the first one.
            const nlohmann::json& manifest = manifests.at(0);

            std::vector<std::string> layerPaths;
            for (const auto& layerPath : manifest.at("Layers"))
            {
                layerPaths.push_back(layerPath.get<std::string>());
            }

            return ManifestDetails{
                .configPath = manifest.at("Config").get<std::string>(),
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

    ImageParser::ImageParser(ImageParserOptions options) : m_options(std::move(options))
    {
        if (m_options.workerCount != 1)
        {
            const size_t chunkSize = std::max<size_t>(1, m_options.chunkSize);
            const size_t maxChunks = static_cast<size_t>(std::max<uint64_t>(1, m_options.readAheadByteBudget / chunkSize));

            m_chunkPool.emplace(chunkSize, maxChunks, m_options.stopToken);
            m_workerPool.emplace(m_options.workerCount);
        }
    }

    ImageParser::~ImageParser()
    {
        // Workers touch members declared below the pools, which would otherwise be destroyed first.
        if (m_workerPool)
        {
            m_workerPool->WaitForIdle();
        }
    }

    void ImageParser::ProcessEntry(ArchiveEntry& entry)
    {
        const std::string_view pathname = entry.Pathname();

        if (pathname == "manifest.json")
        {
            ManifestDetails manifest = ParseManifestJson(entry);

            m_configPath = std::move(manifest.configPath);
            m_layerPaths = std::move(manifest.layerPaths);
        }
        else if (pathname.starts_with(BlobPrefix))
        {
            ProcessBlob(entry);
        }
    }

    void ImageParser::ProcessBlob(ArchiveEntry& entry)
    {
        // Sniff a small, bounded prefix first - large layer blobs must never be buffered in
        // full just to find out they're not JSON.
        std::array<char, SniffLength> sniffBuffer{};
        const size_t sniffLength = std::min(sniffBuffer.size(), static_cast<size_t>(entry.Size()));
        const std::span<char> sniffSpan = std::span{sniffBuffer}.first(sniffLength);

        const size_t sniffed = entry.ReadInto(std::as_writable_bytes(sniffSpan));
        const std::span<const std::byte> prefix = std::as_bytes(std::span{sniffBuffer}).first(sniffed);

        if (LooksLikeJsonObject(std::string_view{sniffBuffer.data(), sniffed}))
        {
            // Confirmed JSON (manifest/config/etc. - always small) - now safe to buffer in full.
            std::string contents(sniffBuffer.data(), sniffed);
            contents.resize(entry.Size());

            const std::span<char> remainingSpan = std::span{contents}.subspan(sniffed);
            const size_t read = entry.ReadInto(std::as_writable_bytes(remainingSpan));
            contents.resize(sniffed + read);

            m_jsonBlobsByDigest.emplace(std::string{entry.Pathname()}, std::move(contents));

            return;
        }

        // Otherwise it is a tar of layer diffs.
        std::string archivePath{entry.Pathname()};

        if (m_workerPool)
        {
            StreamLayerToWorker(entry, std::move(archivePath), prefix);
        }
        else
        {
            ParseLayerInline(entry, std::move(archivePath), prefix);
        }
    }

    void ImageParser::ParseLayerInline(ArchiveEntry& entry, std::string archivePath,
                                       const std::span<const std::byte> prefix)
    {
        EntryByteStream stream{&entry, prefix};
        auto reader = ArchiveReader::CreateFromStream(stream, m_options.stopToken);

        StoreLayer(std::move(archivePath), ParseLayerFilesystem(&reader));
    }

    void ImageParser::StreamLayerToWorker(ArchiveEntry& entry, std::string archivePath,
                                          const std::span<const std::byte> prefix)
    {
        AcquireLayerSlot();

        auto stream = std::make_shared<ChunkQueueByteStream>(*m_chunkPool, entry.Size(), m_options.stopToken);

        // Submitted before a single chunk has been filled, so the worker is inflating the head of the
        // layer while the walk is still reading its tail.
        m_workerPool->Submit([this, stream, path = std::move(archivePath)]() mutable {
            try
            {
                auto reader = ArchiveReader::CreateFromStream(*stream, m_options.stopToken);
                StoreLayer(std::move(path), ParseLayerFilesystem(&reader));
            }
            catch (...)
            {
                RecordWorkerException(std::current_exception());
            }

            // Both must happen however the parse ended: the first so the walk stops queueing chunks
            // nobody will drain, the second so the next layer can start.
            stream->ConsumerFinished();
            ReleaseLayerSlot();
        });

        size_t prefixOffset = 0;

        while (true)
        {
            Chunk chunk = m_chunkPool->Acquire();

            if (chunk.buffer.empty())
            {
                // Cancelled.
                break;
            }

            size_t filled = 0;

            if (prefixOffset < prefix.size())
            {
                filled = std::min(prefix.size() - prefixOffset, chunk.buffer.size());
                std::copy_n(prefix.begin() + prefixOffset, filled, chunk.buffer.begin());
                prefixOffset += filled;
            }

            filled += entry.ReadInto(std::span{chunk.buffer}.subspan(filled));

            // A short fill means the entry's body is exhausted.
            const bool lastChunk = filled < chunk.buffer.size();

            chunk.used = filled;

            const bool wanted = stream->Push(std::move(chunk));

            if (lastChunk || !wanted)
            {
                // !wanted means the worker stopped reading early - libarchive stops at the end of the
                // gzip member and ignores any trailing padding - so the rest of the entry can just be
                // skipped by the outer reader rather than copied.
                break;
            }
        }

        stream->Finish();
    }

    void ImageParser::StoreLayer(std::string archivePath, FilesystemChangeTreeNodeRef tree)
    {
        const std::scoped_lock lock{m_resultsMutex};
        m_filesystemsByLayerDigest.emplace(std::move(archivePath), std::move(tree));
    }

    void ImageParser::RecordWorkerException(std::exception_ptr exception)
    {
        const std::scoped_lock lock{m_resultsMutex};

        if (!m_firstWorkerException)
        {
            m_firstWorkerException = std::move(exception);
        }
    }

    void ImageParser::AcquireLayerSlot()
    {
        const size_t limit = m_workerPool->WorkerCount();

        std::unique_lock lock{m_slotMutex};

        m_slotFreed.wait(lock, m_options.stopToken, [this, limit] { return m_layersInFlight < limit; });

        m_layersInFlight += 1;
    }

    void ImageParser::ReleaseLayerSlot()
    {
        {
            const std::scoped_lock lock{m_slotMutex};
            m_layersInFlight -= 1;
        }

        m_slotFreed.notify_one();
    }

    std::vector<ImageLayer> ImageParser::Build()
    {
        if (m_workerPool)
        {
            m_workerPool->WaitForIdle();
        }

        if (m_firstWorkerException)
        {
            std::rethrow_exception(m_firstWorkerException);
        }

        std::vector<ImageLayer> layers;

        if (!m_configPath)
        {
            return layers;
        }

        const auto configIt = m_jsonBlobsByDigest.find(*m_configPath);
        if (configIt != m_jsonBlobsByDigest.end())
        {
            const nlohmann::json config = nlohmann::json::parse(configIt->second);
            const nlohmann::json& historyArray = config.at("history");

            size_t layerNum = 0;
            FilesystemChangeTreeNodeRef currentFsSnapshot{nullptr};

            for (const auto& historyObj : historyArray)
            {
                if (auto it = historyObj.find("empty_layer"); it != historyObj.end() && it->get<bool>())
                {
                    continue;
                }

                std::string_view command = historyObj.at("created_by").get_ref<const std::string&>();

                // TODO: Make this more destructive so the filesystem tree can be moved?
                // Though, does it really matter if it's just a shared pointer?
                const std::string& layerPath = m_layerPaths.at(layerNum);
                const FilesystemChangeTreeNodeRef& diff = m_filesystemsByLayerDigest.at(layerPath);

                FilesystemChangeTreeNodeRef fs;

                if (currentFsSnapshot)
                {
                    fs = ApplyFilesystemChanges(currentFsSnapshot, diff);
                }
                else
                {
                    fs = diff;
                }

                layers.emplace_back(CollapseWhitespace(command), fs);

                layerNum += 1;

                currentFsSnapshot = Visit(fs, [](const FilesystemChangeInfo& info) {
                    if (info.changeKind != FilesystemChangeKind::None)
                    {
                        return FilesystemChangeInfo{
                            .kind = info.kind,
                            .size = info.size,
                            .changeKind = FilesystemChangeKind::None,
                        };
                    }
                    else
                    {
                        return info;
                    }
                });
            }
        }

        return layers;
    }
} // namespace Flense::Core
