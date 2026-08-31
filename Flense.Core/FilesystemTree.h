#pragma once

#include "FileKind.h"
#include "Tree.h"

#include <cstdint>

namespace Flense::Core
{

    /// <summary>
    /// Represents a type of diff operation on a file in a filesystem tree.
    /// </summary>
    enum class FilesystemChangeKind
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
        FileKind kind;
        uint64_t size;
        FilesystemChangeKind changeKind;

        bool operator==(const FilesystemChangeInfo& other) const = default;
    };

    /// <summary>
    /// A node in a tree that describes diffs applied to a filesystem.
    /// </summary>
    using FilesystemChangeTreeNode = typename TreeNode<FilesystemChangeInfo>;

    /// <summary>
    /// A ref to a node in a tree that describes diffs applied to a filesystem.
    /// </summary>
    using FilesystemChangeTreeNodeRef = typename TreeNodeRef<FilesystemChangeInfo>;

} // namespace Flense::Core