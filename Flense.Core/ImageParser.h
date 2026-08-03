#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Flense::Core
{
    class ArchiveEntry;

    class ImageLayer
    {
      public:
        ImageLayer(std::string digest, uint64_t size) : m_digest(std::move(digest)), m_size(size)
        {
        }

        std::string_view Digest() const
        {
            return m_digest;
        }

        uint64_t Size() const
        {
            return m_size;
        }

        // TODO: populate from the image config's "history" array once that's parsed.
        std::string_view Command() const
        {
            return m_command;
        }

      private:
        std::string m_digest;
        uint64_t m_size;
        std::string m_command;
    };

    /// <summary>
    /// Accumulates OCI image manifest/config details from an archive's entries, fed one at a time -
    /// e.g. alongside a caller-owned loop that's also driving ArchiveReader::Next() for progress
    /// reporting, since only one consumer can call Next() on a given ArchiveReader.
    /// </summary>
    class ImageParser
    {
      public:
        void ProcessEntry(const ArchiveEntry& entry);

        /// <summary>
        /// Resolves everything accumulated so far into the image's layer list. Call once the
        /// archive has been fully enumerated.
        /// </summary>
        std::vector<ImageLayer> Build() const;

      private:
        std::optional<std::string> m_manifestDigest;
        std::unordered_map<std::string, nlohmann::json> m_jsonBlobsByDigest;
    };

} // namespace Flense::Core