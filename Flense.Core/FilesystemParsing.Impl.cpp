module Flense.Core.ImageParsing:FilesystemParsing.Impl;

import :ArchiveReader;
import :FilesystemParsing;
import :Filesystem;
import :Tree;
import std;

namespace Flense::Core
{
    namespace
    {
        struct MutableTreeNode
        {
            FilesystemChangeInfo info{};
            std::flat_map<std::string, MutableTreeNode, std::less<>> children{};
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

            std::size_t baseIndex = 0;
            std::size_t diffIndex = 0;

            const bool opaqueWhiteout = std::ranges::any_of(diffChildren.values(), [](const auto& diffChild) {
                return diffChild->Data().changeKind == FilesystemChangeKind::OpaqueWhiteout;
            });

            while (baseIndex < baseKeys.size() || diffIndex < diffKeys.size())
            {
                const bool baseExhausted = baseIndex >= baseKeys.size();
                const bool diffExhausted = diffIndex >= diffKeys.size();

                if (diffExhausted || (!baseExhausted && baseKeys.at(baseIndex) < diffKeys.at(diffIndex)))
                {
                    // Only in base.
                    const FilesystemChangeTreeNodeRef& baseValue = baseValues.at(baseIndex);

                    // Do one of three things:
                    //
                    // 1) Drop nodes currently marked as deleted in the base tree, so that they don't show in subsequent
                    // layers.
                    //
                    // 2) If there is an opaque whiteout in the diff, mark all base nodes as deleted by it.
                    //
                    // 3) In all other cases, clear the change kind the nodes had and mark them as unmodified. Note that
                    // it's fine to do this every time since Visit will reuse value-equal nodes.

                    auto newValue = VisitWithPruning(
                        baseValue,
                        [opaqueWhiteout](const FilesystemChangeInfo& info) -> std::optional<FilesystemChangeInfo> {
                            if (info.changeKind == FilesystemChangeKind::Removed)
                            {
                                return std::nullopt;
                            }

                            FilesystemChangeInfo newInfo = info;
                            newInfo.changeKind =
                                opaqueWhiteout ? FilesystemChangeKind::Removed : FilesystemChangeKind::None;

                            return newInfo;
                        });

                    if (newValue)
                    {
                        outKeys.push_back(baseKeys.at(baseIndex));
                        outValues.push_back(std::move(*newValue));
                    }

                    ++baseIndex;
                }
                else if (baseExhausted || diffKeys.at(diffIndex) < baseKeys.at(baseIndex))
                {
                    // Only in diff: a brand new entry. Nothing to remove it from, so skip whiteouts here.
                    if (auto changeKind = diffValues.at(diffIndex)->Data().changeKind;
                        changeKind != FilesystemChangeKind::Removed &&
                        changeKind != FilesystemChangeKind::OpaqueWhiteout)
                    {
                        outKeys.push_back(diffKeys.at(diffIndex));
                        outValues.push_back(diffValues.at(diffIndex));
                    }
                    ++diffIndex;
                }
                else
                {
                    // Present in both: recursively patch;
                    auto patched = PatchNode(baseValues.at(baseIndex), diffValues.at(diffIndex));
                    outKeys.push_back(baseKeys.at(baseIndex));
                    outValues.push_back(std::move(patched));

                    ++baseIndex;
                    ++diffIndex;
                }
            }

            return {std::sorted_unique, std::move(outKeys), std::move(outValues)};
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
                    FilesystemChangeInfo infoCopy = info;
                    infoCopy.changeKind = FilesystemChangeKind::Removed;
                    return infoCopy;
                });
            }

            // The root node exists in both trees, so it is not strictly added but rather modified.
            FilesystemChangeInfo newInfo(diff->Data());
            newInfo.changeKind = FilesystemChangeKind::Modified;

            const FilesystemChangeTreeNode::ChildrenContainer children =
                PatchChildren(base->Children(), diff->Children());

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
        auto root = FilesystemChangeInfo{
            .kind = FileKind::Directory,
            .size = 0,
            .changeKind = FilesystemChangeKind::Added,
            .linkTarget = std::nullopt,
        };

        auto rootNode = MutableTreeNode{
            .info = root,
        };

        while (auto entry = nestedTarReader->Next(stopToken))
        {
            const std::string_view path = entry->Pathname();

            auto split = std::views::split(path, '/') |
                         std::views::transform([](auto t) { return std::string_view(t); }) |
                         std::views::filter([](std::string_view c) { return !c.empty() && c != "."; });

            // The root is a synthetic directory used to aggregate this layer.  Archive metadata can
            // contain entries such as "" or "."; neither identifies a filesystem node and must not
            // overwrite the root's directory kind or aggregate size.
            if (split.begin() == split.end())
            {
                continue;
            }

            MutableTreeNode* node = &rootNode;

            for (const std::string_view pathComponent : split)
            {
                std::string_view pathString = pathComponent;
                FilesystemChangeKind changeKind{};

                static constexpr std::string_view WhiteoutPrefix = ".wh.";
                static constexpr std::string_view OpaqueWhiteoutName = ".wh..wh..opq";

                if (pathComponent == OpaqueWhiteoutName)
                {
                    changeKind = FilesystemChangeKind::OpaqueWhiteout;
                }
                else if (pathComponent.starts_with(WhiteoutPrefix) && pathComponent.size() > WhiteoutPrefix.size())
                {
                    pathString = pathComponent.substr(WhiteoutPrefix.size());
                    changeKind = FilesystemChangeKind::Removed;
                }
                else
                {
                    changeKind = FilesystemChangeKind::Added;
                }

                auto [it, added] = node->children.try_emplace(pathString, FilesystemChangeInfo{
                                                                              .kind = FileKind::Directory,
                                                                              .size = 0,
                                                                              .changeKind = changeKind,
                                                                              .linkTarget = std::nullopt,
                                                                          });

                node = &it->second;
            }

            node->info.kind = entry->FileKind();
            node->info.size = entry->Size();

            if (node->info.kind == FileKind::Symlink)
            {
                node->info.linkTarget = entry->Symlink();
            }
            else if (node->info.kind == FileKind::Hardlink)
            {
                node->info.linkTarget = entry->Hardlink();
            }
            else
            {
                // We might be overwriting an existing node if the same path appears twice in the archive,
                // so we should reset here to ensure this field is not set for non link files.
                node->info.linkTarget = std::nullopt;
            }
        }

        return Freeze(std::move(rootNode));
    }

    FilesystemChangeTreeNodeRef ApplyFilesystemChanges(const FilesystemChangeTreeNodeRef& base,
                                                       const FilesystemChangeTreeNodeRef& diff)
    {
        return PatchNode(base, diff);
    }
} // namespace Flense::Core
