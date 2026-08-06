#include "pch.h"

#include "ArchiveReader.h"
#include "FilesystemTree.h"
#include "ParseFilesystemTree.h"

#include <ranges>

namespace Flense::Core
{
    namespace
    {
        bool CompareNodeDataByName(const FilesystemNodeInfo& a, const FilesystemNodeInfo& b)
        {
            return a.name == b.name;
        }
    } // namespace

    FilesystemChangeTreeNodeRef ParseLayerFilesystem(ArchiveReader* nestedTarReader)
    {
        while (auto entry = nestedTarReader->Next())
        {
            auto split = std::views::split(entry->Pathname(), '/');

            for (auto it = split.begin(); it != split.end(); it++)
            {
            }
        }
    }
} // namespace Flense::Core
