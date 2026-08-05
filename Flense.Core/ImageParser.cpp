#include "pch.h"

#include "ArchiveReader.h"
#include "ImageLayer.h"
#include "ImageParser.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
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
            std::string digest;
            std::string contents;
        };

        /// <summary>
        /// A binary filesystem blob. Stub implementation.
        /// </summary>
        struct BlobFsDetails
        {
        };

        using ParsedEntry = std::variant<std::monostate, ManifestDetails, JsonBlobDetails, BlobFsDetails>;

        constexpr std::string_view BlobPrefix = "blobs/sha256/";

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

            std::span<char> const contentsSpan = std::span{contents};
            size_t const bytesRead = entry.ReadInto(std::as_writable_bytes(contentsSpan));
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
        /// <returns>A BlobFsDetails or JsonBlobDetails.</returns>
        ParsedEntry ParseBlob(const ArchiveEntry& entry)
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

                std::span<char> const remainingSpan = std::span{contents}.subspan(sniffed);
                const size_t read = entry.ReadInto(std::as_writable_bytes(remainingSpan));
                contents.resize(sniffed + read);

                return JsonBlobDetails{.digest = std::string{entry.Pathname()}, .contents = std::move(contents)};
            }
            else
            {
                // This is a tar file containing layer diffs.
                return BlobFsDetails{};
            }
        }

        /// <summary>
        /// Parses an archive entry into one of a number of possible entry types.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        /// <returns>A variant over the possible types.</returns>
        ParsedEntry ParseEntry(const ArchiveEntry& entry)
        {
            const std::string_view pathname = entry.Pathname();

            if (pathname == "manifest.json")
            {
                return ParseManifestJson(entry);
            }

            if (pathname.starts_with(BlobPrefix))
            {
                return ParseBlob(entry);
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

    void ImageParser::ProcessEntry(const ArchiveEntry& entry)
    {
        ParsedEntry parsed = ParseEntry(entry);

        std::visit(
            [this]<typename T>(T& value) {
                if constexpr (std::is_same_v<T, ManifestDetails>)
                {
                    m_configPath = std::move(value.configPath);
                    m_layerPaths = std::move(value.layerPaths);
                }
                else if constexpr (std::is_same_v<T, JsonBlobDetails>)
                {
                    m_jsonBlobsByDigest.emplace(std::move(value.digest), std::move(value.contents));
                }
                else if constexpr (std::is_same_v<T, BlobFsDetails> || std::is_same_v<T, std::monostate>)
                {
                    // Nothing to accumulate (yet).
                }
                else
                {
                    static_assert(false, "unhandled ParsedEntry alternative");
                }
            },
            parsed);
    }

    std::vector<ImageLayer> ImageParser::Build() const
    {
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

            for (const auto& historyObj : historyArray)
            {
                if (auto it = historyObj.find("empty_layer"); it != historyObj.end() && it->get<bool>())
                {
                    continue;
                }

                std::string_view command = historyObj.at("created_by").get_ref<const std::string&>();

                layers.emplace_back(CollapseWhitespace(command));
            }
        }

        return layers;
    }
} // namespace Flense::Core
