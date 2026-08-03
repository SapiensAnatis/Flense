#include "pch.h"

#include "ArchiveReader.h"
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
        // OCI digests are "<algorithm>:<hex>" (e.g. "sha256:abcd..."); blob paths are
        // "blobs/<algorithm>/<hex>". Only sha256 is supported for now.
        std::string GetDigestHash(const std::string_view digest)
        {
            const auto colon = digest.find(':');
            return std::string{colon == std::string_view::npos ? digest : digest.substr(colon + 1)};
        }

        bool LooksLikeJson(const std::string_view prefix)
        {
            const auto it =
                std::ranges::find_if(prefix, [](char c) { return !std::isspace(static_cast<unsigned char>(c)); });
            return it != prefix.end() && *it == '{';
        }

        // index.json points at the (or, for multi-arch images, one of several) image manifest blob(s).
        struct IndexDetails
        {
            std::string manifestDigest;
        };

        // Any JSON-shaped blob under blobs/sha256/ - could be the image manifest, the image config,
        // or something else entirely. Its role isn't known until it's cross-referenced against
        // IndexDetails/another manifest, so it's kept generic rather than guessed at here.
        struct JsonBlobDetails
        {
            std::string digest;
            nlohmann::json json;
        };

        // A binary blob (almost certainly a layer). Stub for now - walking its contents (as a nested
        // tar) to list files will come later.
        struct BlobFsDetails
        {
        };

        using ParsedEntry = std::variant<std::monostate, IndexDetails, JsonBlobDetails, BlobFsDetails>;

        ParsedEntry ParseEntry(const ArchiveEntry& entry)
        {
            const std::string_view pathname = entry.Pathname();

            if (pathname == "index.json")
            {
                std::string contents;
                contents.resize(entry.Size());

                std::span<char> const contentsSpan = std::span{contents};
                size_t const bytesRead = entry.ReadInto(std::as_writable_bytes(contentsSpan));
                contents.resize(bytesRead);

                nlohmann::json const index = nlohmann::json::parse(contents);

                // TODO: support multi-arch images (index.json can list manifests for multiple
                // platforms) - for now we just take the first one.
                return IndexDetails{
                    .manifestDigest = GetDigestHash(index.at("manifests").at(0).at("digest").get<std::string>()),
                };
            }

            constexpr std::string_view BlobPrefix = "blobs/sha256/";
            if (!pathname.starts_with(BlobPrefix))
            {
                return std::monostate{};
            }

            const std::string digest{pathname.substr(BlobPrefix.size())};

            // Sniff a small, bounded prefix first - large layer blobs must never be buffered in
            // full just to find out they're not JSON.
            std::array<char, 64> sniffBuffer{};
            const size_t sniffLength = std::min(sniffBuffer.size(), static_cast<size_t>(entry.Size()));
            std::span<char> sniffSpan = std::span{sniffBuffer}.first(sniffLength);

            const size_t sniffed = entry.ReadInto(std::as_writable_bytes(sniffSpan));

            if (!LooksLikeJson(std::string_view{sniffBuffer.data(), sniffed}))
            {
                return BlobFsDetails{};
            }

            // Confirmed JSON (manifest/config/etc. - always small) - now safe to buffer in full.
            std::string contents(sniffBuffer.data(), sniffed);
            contents.resize(entry.Size());

            std::span<char> const remainingSpan = std::span{contents}.subspan(sniffed);
            const size_t read = entry.ReadInto(std::as_writable_bytes(remainingSpan));
            contents.resize(sniffed + read);

            nlohmann::json json = nlohmann::json::parse(contents);

            return JsonBlobDetails{.digest = digest, .json = std::move(json)};
        }
    } // namespace

    void ImageParser::ProcessEntry(const ArchiveEntry& entry)
    {
        ParsedEntry parsed = ParseEntry(entry);

        std::visit(
            [&]<typename T>(T& value) {
                if constexpr (std::is_same_v<T, IndexDetails>)
                {
                    m_manifestDigest = std::move(value.manifestDigest);
                }
                else if constexpr (std::is_same_v<T, JsonBlobDetails>)
                {
                    m_jsonBlobsByDigest.emplace(std::move(value.digest), std::move(value.json));
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
        if (!m_manifestDigest)
        {
            return std::vector<ImageLayer>();
        }

        auto const manifestIt = m_jsonBlobsByDigest.find(*m_manifestDigest);
        if (manifestIt == m_jsonBlobsByDigest.end())
        {
            return std::vector<ImageLayer>();
        }

        std::vector<ImageLayer> layers;
        for (auto const& layer : manifestIt->second.at("layers"))
        {
            layers.emplace_back(GetDigestHash(layer.at("digest").get<std::string>()), layer.at("size").get<uint64_t>());
        }

        return layers;
    }
} // namespace Flense::Core
