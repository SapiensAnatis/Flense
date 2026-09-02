export module Flense.Core:FilesystemTree;

import :FileKind;
import :Tree;
import std;

export namespace Flense::Core
{

    /// <summary>
    /// Represents a type of diff operation on a file in a filesystem tree.
    /// </summary>
    enum class FilesystemChangeKind : std::uint8_t
    {
        Unspecified = 0,
        None,
        Added,
        Removed,
        Modified,
    };

    /// <summary>
    /// Represents a diff that was applied to a filesystem node.
    /// </summary>
    struct FilesystemChangeInfo
    {
        FileKind kind{};
        std::uint64_t size{};
        FilesystemChangeKind changeKind{};

        bool operator==(const FilesystemChangeInfo& other) const = default;
    };

    /// <summary>
    /// A node in a tree that describes diffs applied to a filesystem.
    /// </summary>
    using FilesystemChangeTreeNode = TreeNode<FilesystemChangeInfo>;

    /// <summary>
    /// A ref to a node in a tree that describes diffs applied to a filesystem.
    /// </summary>
    using FilesystemChangeTreeNodeRef = TreeNodeRef<FilesystemChangeInfo>;

} // namespace Flense::Core
