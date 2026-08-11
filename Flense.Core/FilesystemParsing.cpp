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
        /// Recursively drops any nodes marked FilesystemChangeKind::Removed from a diff subtree that is being
        /// inserted wholesale, since there is nothing in the (absent) base for them to remove.
        /// </summary>
        FilesystemChangeTreeNodeRef PruneRemoved(const FilesystemChangeTreeNodeRef& node)
        {
            const auto& keys = node->Children().keys();
            const auto& values = node->Children().values();

            std::vector<std::string> prunedKeys;
            std::vector<FilesystemChangeTreeNodeRef> prunedValues;

            bool changed = false;

            for (size_t i = 0; i < keys.size(); ++i)
            {
                if (values[i]->Data().changeKind == FilesystemChangeKind::Removed)
                {
                    changed = true;
                    continue;
                }

                auto prunedChild = PruneRemoved(values[i]);
                changed = changed || (prunedChild.get() != values[i].get());

                prunedKeys.push_back(keys[i]);
                prunedValues.push_back(std::move(prunedChild));
            }

            if (!changed)
            {
                return node; // no removed descendants, alias the existing subtree
            }

            return FilesystemChangeTreeNode::Create(
                node->Data(), FilesystemChangeTreeNode::ChildrenContainer(std::sorted_unique, std::move(prunedKeys),
                                                                          std::move(prunedValues)));
        }

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
                        outValues.push_back(PruneRemoved(diffValues[diffIndex]));
                    }
                    ++diffIndex;
                }
                else
                {
                    // Present in both: recursively patch, unless the diff removes this node entirely.
                    if (auto patched = PatchNode(baseValues[baseIndex], diffValues[diffIndex]))
                    {
                        outKeys.push_back(baseKeys[baseIndex]);
                        outValues.push_back(std::move(patched));
                    }

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
        /// <returns>The patched node, or nullptr if the diff removes this node entirely.</returns>
        FilesystemChangeTreeNodeRef PatchNode(const FilesystemChangeTreeNodeRef& base,
                                              const FilesystemChangeTreeNodeRef& diff)
        {
            if (diff->Data().changeKind == FilesystemChangeKind::Removed)
            {
                return nullptr;
            }

            // The root node exists in both trees, so it is not strictly added but rather modified.
            FilesystemChangeInfo newInfo(diff->Data());
            newInfo.changeKind = FilesystemChangeKind::Modified;
            newInfo.size += base->Data().size;

            return FilesystemChangeTreeNode::Create(newInfo, PatchChildren(base->Children(), diff->Children()));
        }
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

        return Freeze(std::move(rootNode));
    }

    FilesystemChangeTreeNodeRef ApplyFilesystemChanges(const FilesystemChangeTreeNodeRef& base,
                                                       const FilesystemChangeTreeNodeRef& diff)
    {
        return PatchNode(base, diff);
    }
} // namespace Flense::Core
