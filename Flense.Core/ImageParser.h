#pragma once

#include "FilesystemTree.h"

#include <cstdint>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Flense::Core
{
    class ArchiveEntry;
    class ImageLayer;

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
        /// <summary>
        /// Processes an individual archive entry, and store it in the parser's internal state for a later Build() call.
        /// </summary>
        /// <param name="entry">The archive entry.</param>
        /// <param name="stopToken">The token to check for cancellation. Parsing a layer blob can take a long time, so
        /// it is checked once per entry within the layer's nested archive rather than only between top-level entries.
        /// If cancellation is requested part-way through, the parser is left holding partial state and Build() must
        /// not be called.</param>
        void ProcessEntry(ArchiveEntry& entry, std::stop_token stopToken);

        /// <summary>
        /// Assembles a vector of image layers once all archive entries have been handed to ProcessEntry().
        /// </summary>
        /// <returns>A vector of image layers.</returns>
        std::vector<ImageLayer> Build() const;

      private:
        std::optional<std::string> m_configPath;
        std::vector<std::string> m_layerPaths;

        // Raw, not-yet-parsed JSON text keyed by digest hashes
        std::unordered_map<std::string, std::string> m_jsonBlobsByDigest;
        std::unordered_map<std::string, FilesystemChangeTreeNodeRef> m_filesystemsByLayerDigest;
    };

} // namespace Flense::Core