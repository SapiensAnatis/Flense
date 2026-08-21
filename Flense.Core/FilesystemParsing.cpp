#include "pch.h"

#include "ArchiveReader.h"
#include "FilesystemParsing.h"
#include "FilesystemTree.h"

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

        TreeNodeRef<FilesystemChangeInfo> Freeze(MutableTreeNode&& node)
        {
            auto containers = std::move(node.children).extract();

            std::vector<FilesystemChangeTreeNodeRef> frozen;
            frozen.reserve(containers.values.size());

            for (auto& child : containers.values)
            {
                const FilesystemChangeTreeNodeRef& added = frozen.emplace_back(Freeze(std::move(child)));
                node.info.size += added->Data().size;
            }

            return FilesystemChangeTreeNode::Create(
                node.info, std::flat_map<std::string, FilesystemChangeTreeNodeRef>(
                               std::sorted_unique, std::move(containers.keys), std::move(frozen)));
        }

        // TODO: This patch logic is entirely AI-generated. Check it over when tidying up the code

        FilesystemChangeTreeNodeRef PatchNode(const FilesystemChangeTreeNodeRef& base,
                                              const FilesystemChangeTreeNodeRef& diff);

        /// <summary>
        /// Merges a base node's children with a diff node's children: entries only in base carry over unchanged,
        /// entries only in diff are inserted wholesale, and entries in both are recursively patched.
        /// </summary>
        FilesystemChangeTreeNode::ChildrenContainer PatchChildren(
            const FilesystemChangeTreeNode::ChildrenContainer& baseChildren,
            const FilesystemChangeTreeNode::ChildrenContainer& diffChildren)
        {
            const auto& baseKeys = baseChildren.keys();
            const auto& baseValues = baseChildren.values();
            const auto& diffKeys = diffChildren.keys();
            const auto& diffValues = diffChildren.values();

            std::vector<std::string> outKeys;
            std::vector<FilesystemChangeTreeNodeRef> outValues;
            outKeys.reserve(baseKeys.size() + diffKeys.size());
            outValues.reserve(baseKeys.size() + diffKeys.size());

            size_t baseIndex = 0;
            size_t diffIndex = 0;

            while (baseIndex < baseKeys.size() || diffIndex < diffKeys.size())
            {
                const bool baseExhausted = baseIndex >= baseKeys.size();
                const bool diffExhausted = diffIndex >= diffKeys.size();

                if (diffExhausted || (!baseExhausted && baseKeys[baseIndex] < diffKeys[diffIndex]))
                {
                    // Only in base: not touched by this diff, carry over unchanged.
                    outKeys.push_back(baseKeys[baseIndex]);
                    outValues.push_back(baseValues[baseIndex]);
                    ++baseIndex;
                }
                else if (baseExhausted || diffKeys[diffIndex] < baseKeys[baseIndex])
                {
                    // Only in diff: a brand new entry. Nothing to remove it from, so skip whiteouts here.
                    if (diffValues[diffIndex]->Data().changeKind != FilesystemChangeKind::Removed)
                    {
                        outKeys.push_back(diffKeys[diffIndex]);
                        outValues.push_back(diffValues[diffIndex]);
                    }
                    ++diffIndex;
                }
                else
                {
                    // Present in both: recursively patch;
                    auto patched = PatchNode(baseValues[baseIndex], diffValues[diffIndex]);
                    outKeys.push_back(baseKeys[baseIndex]);
                    outValues.push_back(std::move(patched));

                    ++baseIndex;
                    ++diffIndex;
                }
            }

            return FilesystemChangeTreeNode::ChildrenContainer(std::sorted_unique, std::move(outKeys),
                                                               std::move(outValues));
        }

        /// <summary>
        /// Patches a single base node with its corresponding diff node.
        /// </summary>
        /// <returns>The patched node.</returns>
        FilesystemChangeTreeNodeRef PatchNode(const FilesystemChangeTreeNodeRef& base,
                                              const FilesystemChangeTreeNodeRef& diff)
        {
            if (diff->Data().changeKind == FilesystemChangeKind::Removed)
            {
                // Set all children to be removed
                return Visit(base, [](const FilesystemChangeInfo& info) {
                    return FilesystemChangeInfo{
                        .kind = info.kind,
                        .size = info.size,
                        .changeKind = FilesystemChangeKind::Removed,
                    };
                });
            }

            // The root node exists in both trees, so it is not strictly added but rather modified.
            FilesystemChangeInfo newInfo(diff->Data());
            newInfo.changeKind = FilesystemChangeKind::Modified;

            FilesystemChangeTreeNode::ChildrenContainer children = PatchChildren(base->Children(), diff->Children());

            if (newInfo.kind == FileKind::Directory)
            {
                // Roll up the whole flattened subtree, not just this layer's contribution.
                newInfo.size = 0;
                for (const auto& child : children.values())
                {
                    newInfo.size += child->Data().size;
                }
            }

            return FilesystemChangeTreeNode::Create(newInfo, children);
        }
    } // namespace

    FilesystemChangeTreeNodeRef ParseLayerFilesystem(ArchiveReader* nestedTarReader, std::stop_token stopToken)
    {
        static constexpr std::string_view WhiteoutPrefix = ".wh.";

        auto root = FilesystemChangeInfo{
            .kind = FileKind::Directory,
            .size = 0,
            .changeKind = FilesystemChangeKind::Added,
        };

        auto rootNode = MutableTreeNode{
            .info = root,
        };

        while (auto entry = nestedTarReader->Next(stopToken))
        {
            std::string_view path = entry->Pathname();

            // TODO: handle opaque whiteouts, which hide every entry a lower layer put in this directory.
            if (path.ends_with("/.wh..wh..opq"))
            {
                continue;
            }

            auto split = std::views::split(path, '/') |
                         std::views::transform([](auto t) { return std::string_view(t); }) |
                         std::views::filter([](std::string_view c) { return !c.empty() && c != "."; });

            MutableTreeNode* node = &rootNode;

            for (const std::string_view pathComponent : split)
            {
                std::string pathString;
                FilesystemChangeKind changeKind{};

                if (pathComponent.starts_with(WhiteoutPrefix) && pathComponent.size() > WhiteoutPrefix.size())
                {
                    pathString = std::string(pathComponent.begin() + WhiteoutPrefix.size(), pathComponent.end());
                    changeKind = FilesystemChangeKind::Removed;
                }
                else
                {
                    pathString = std::string(pathComponent);
                    changeKind = FilesystemChangeKind::Added;
                }

                auto [it, added] = node->children.try_emplace(pathString, FilesystemChangeInfo{
                                                                              .kind = FileKind::Directory,
                                                                              .size = 0,
                                                                              .changeKind = changeKind,
                                                                          });

                node = &it->second;
            }

            // TODO: handle symlinks
            node->info.kind = entry->FileKind();
            node->info.size = entry->Size();
        }

        return Freeze(std::move(rootNode));
    }

    FilesystemChangeTreeNodeRef ApplyFilesystemChanges(const FilesystemChangeTreeNodeRef& base,
                                                       const FilesystemChangeTreeNodeRef& diff)
    {
        return PatchNode(base, diff);
    }
} // namespace Flense::Core
