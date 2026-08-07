#include "pch.h"

#include "ArchiveReader.h"
#include "FilesystemTree.h"
#include "ParseFilesystemTree.h"

#include <flat_map>
#include <ranges>

namespace Flense::Core
{
    namespace
    {
        struct MutableTreeNode
        {
            FilesystemChangeInfo info;
            std::flat_map<std::string, MutableTreeNode> children;
        };
    } // namespace

    FilesystemChangeTreeNodeRef ParseLayerFilesystem(ArchiveReader* nestedTarReader)
    {
        auto root = FilesystemChangeInfo{
            .kind = FileKind::Directory,
            .size = 0,
            .changeKind = FilesystemChangeKind::Added,
        };

        auto rootNode = MutableTreeNode{
            .info = root,
        };

        while (auto entry = nestedTarReader->Next())
        {
            std::string_view path = entry->Pathname();

            auto split = std::views::split(path, '/') |
                         std::views::transform([](auto t) { return std::string_view(t); }) |
                         std::views::filter([](std::string_view c) { return !c.empty() && c != "."; });

            MutableTreeNode* node = &rootNode;

            for (const std::string_view pathComponent : split)
            {
                auto pathString = std::string(pathComponent.begin(), pathComponent.end());

                auto [it, added] = node->children.try_emplace(pathString, FilesystemChangeInfo{
                                                                              .kind = FileKind::Directory,
                                                                              .size = 0,
                                                                              .changeKind = FilesystemChangeKind::Added,
                                                                          });

                node = &it->second;
            }

            // TODO: handle symlinks and whiteouts

            node->info = FilesystemChangeInfo{
                .kind = entry->FileKind(),
                .size = entry->Size(),
                .changeKind = FilesystemChangeKind::Added,
            };
        }

        return {};
    }
} // namespace Flense::Core
