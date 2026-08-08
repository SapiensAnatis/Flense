#include "pch.h"

#include "ArchiveReader.h"
#include "ByteBudget.h"
#include "FilesystemParsing.h"
#include "FilesystemTree.h"
#include "ImageLayer.h"
#include "ImageParser.h"
#include "MemoryByteStream.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <exception>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
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

        /// <summary>
        /// Represents the file contents of a JSON blob from blobs/sha256/.
        /// </summary>
        /// <remarks>
        /// This could be the image config or something else entirely. Its role isn't known until
        /// it's cross-referenced against ManifestDetails.
        /// </remarks>
        struct JsonBlobDetails
        {
            std::string archivePath;
            std::string contents;
        };

        /// <summary>
        /// The still-compressed bytes of a layer blob, lifted out of the archive so that the walk can
        /// move on while the blob is decompressed elsewhere.
        /// </summary>
        /// <remarks>
        /// Carrying these bytes has already been charged to the parser's ByteBudget, and whoever
        /// finishes with them owes it a Release of compressedBytes.size().
        /// </remarks>
        struct LayerBlobDetails
        {
            std::string archivePath;
            std::vector<std::byte> compressedBytes;
        };

        using ParsedEntry = std::variant<std::monostate, ManifestDetails, JsonBlobDetails, LayerBlobDetails>;

        constexpr std::string_view BlobPrefix = "blobs/sha256/";

        /// <summary>
        /// Releases a ByteBudget charge on scope exit unless it has been handed on to someone else.
        /// A charge that escapes both - because an allocation threw between the two - would never be
        /// returned, and the next producer to wait on the budget would wait forever.
        /// </summary>
        class BudgetRelease
        {
          public:
            BudgetRelease(ByteBudget& budget, const uint64_t bytes) : m_budget(&budget), m_bytes(bytes)
            {
            }

            BudgetRelease(const BudgetRelease&) = delete;
            BudgetRelease& operator=(const BudgetRelease&) = delete;

            ~BudgetRelease()
            {
                if (m_budget)
                {
                    m_budget->Release(m_bytes);
                }
            }

            /// <summary>
            /// Gives up responsibility for the charge - the caller now owes the release.
            /// </summary>
            void Detach()
            {
                m_budget = nullptr;
            }

          private:
            ByteBudget* m_budget;
            uint64_t m_bytes;
        };

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
        /// Parses a blob found under sha256/blobs/.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        /// <param name="budget">The budget to charge a layer blob's buffered bytes to.</param>
        /// <returns>A LayerBlobDetails or JsonBlobDetails.</returns>
        ParsedEntry ParseBlob(ArchiveEntry& entry, ByteBudget& budget)
        {
            // Sniff a small, bounded prefix first - large layer blobs must never be buffered in
            // full just to find out they're not JSON.
            std::array<char, 64> sniffBuffer{};
            const size_t sniffLength = std::min(sniffBuffer.size(), static_cast<size_t>(entry.Size()));
            std::span<char> sniffSpan = std::span{sniffBuffer}.first(sniffLength);

            const size_t sniffed = entry.ReadInto(std::as_writable_bytes(sniffSpan));

            if (LooksLikeJsonObject(std::string_view{sniffBuffer.data(), sniffed}))
            {
                // Confirmed JSON (manifest/config/etc. - always small) - now safe to buffer in full.
                std::string contents(sniffBuffer.data(), sniffed);
                contents.resize(entry.Size());

                const std::span<char> remainingSpan = std::span{contents}.subspan(sniffed);
                const size_t read = entry.ReadInto(std::as_writable_bytes(remainingSpan));
                contents.resize(sniffed + read);

                return JsonBlobDetails{
                    .archivePath = std::string{entry.Pathname()},
                    .contents = std::move(contents),
                };
            }
            else
            {
                // This is a tar file containing layer diffs. The outer archive is forward-only, so its
                // bytes have to be taken now, in full, even though the expensive part - inflating them -
                // is going to happen somewhere else. Waiting on the budget first is what stops this from
                // reading the whole image into memory when the workers fall behind.
                const uint64_t size = entry.Size();
                budget.Acquire(size);
                BudgetRelease charge{budget, size};

                std::vector<std::byte> compressedBytes(size);
                const std::span<std::byte> blobSpan{compressedBytes};

                std::copy_n(std::as_bytes(std::span{sniffBuffer}).begin(), sniffed, blobSpan.begin());
                entry.ReadInto(blobSpan.subspan(sniffed));

                LayerBlobDetails details{
                    .archivePath = std::string{entry.Pathname()},
                    .compressedBytes = std::move(compressedBytes),
                };

                // Whoever parses these bytes returns the charge once they are done with them.
                charge.Detach();

                return details;
            }
        }

        /// <summary>
        /// Parses an archive entry into one of a number of possible entry types.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        /// <param name="budget">The budget to charge a layer blob's buffered bytes to.</param>
        /// <returns>A variant over the possible types.</returns>
        ParsedEntry ParseEntry(ArchiveEntry& entry, ByteBudget& budget)
        {
            const std::string_view pathname = entry.Pathname();

            if (pathname == "manifest.json")
            {
                return ParseManifestJson(entry);
            }

            if (pathname.starts_with(BlobPrefix))
            {
                return ParseBlob(entry, budget);
            }

            return std::monostate{};
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

    ImageParser::ImageParser(ImageParserOptions options)
        : m_options(std::move(options)), m_budget(m_options.inFlightByteBudget, m_options.stopToken)
    {
        if (m_options.workerCount != 1)
        {
            m_pool.emplace(m_options.workerCount);
        }
    }

    ImageParser::~ImageParser()
    {
        // Workers touch members declared below m_pool, which would otherwise be destroyed first.
        if (m_pool)
        {
            m_pool->WaitForIdle();
        }
    }

    void ImageParser::ProcessEntry(ArchiveEntry& entry)
    {
        ParsedEntry parsed = ParseEntry(entry, m_budget);

        std::visit(
            [this]<typename T>(T& value) {
                if constexpr (std::is_same_v<T, ManifestDetails>)
                {
                    m_configPath = std::move(value.configPath);
                    m_layerPaths = std::move(value.layerPaths);
                }
                else if constexpr (std::is_same_v<T, JsonBlobDetails>)
                {
                    m_jsonBlobsByDigest.emplace(std::move(value.archivePath), std::move(value.contents));
                }
                else if constexpr (std::is_same_v<T, LayerBlobDetails>)
                {
                    if (m_pool)
                    {
                        m_pool->Submit([this, path = std::move(value.archivePath),
                                        bytes = std::move(value.compressedBytes)]() mutable {
                            ParseLayerBlob(std::move(path), std::move(bytes));
                        });
                    }
                    else
                    {
                        ParseLayerBlob(std::move(value.archivePath), std::move(value.compressedBytes));
                    }
                }
                else if constexpr (std::is_same_v<T, std::monostate>)
                {
                    // Do nothing
                }
                else
                {
                    static_assert(false, "unhandled ParsedEntry alternative");
                }
            },
            parsed);
    }

    void ImageParser::ParseLayerBlob(std::string archivePath, std::vector<std::byte> compressedBytes)
    {
        const BudgetRelease release{m_budget, compressedBytes.size()};

        try
        {
            MemoryByteStream stream{compressedBytes};
            auto reader = ArchiveReader::CreateFromStream(stream, m_options.stopToken);

            FilesystemChangeTreeNodeRef tree = ParseLayerFilesystem(&reader);

            const std::scoped_lock lock{m_resultsMutex};
            m_filesystemsByLayerDigest.emplace(std::move(archivePath), std::move(tree));
        }
        catch (...)
        {
            const std::scoped_lock lock{m_resultsMutex};
            if (!m_firstWorkerException)
            {
                m_firstWorkerException = std::current_exception();
            }
        }
    }

    std::vector<ImageLayer> ImageParser::Build()
    {
        if (m_pool)
        {
            m_pool->WaitForIdle();
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
