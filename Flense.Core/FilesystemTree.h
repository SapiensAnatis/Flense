#pragma once

#include "Tree.h"

#include <string>

namespace Flense::Core
{
    /// <summary>
    /// Represents a type of node in a filesystem tree.
    /// </summary>
    enum class FilesystemNodeKind
    {
        Directory = 0,
        File = 1,
    };

    /// <summary>
    /// Represents a type of diff operation on a file in a filesystem tree.
    /// </summary>
    enum class FilesystemChangeKind
    {
        None = 0,
        Added = 1,
        Removed = 2,
    };

    /// <summary>
    /// Represents the information associated with a filesystem tree node.
    /// </summary>
    struct FilesystemNodeInfo
    {
        FilesystemNodeKind kind;
        uint64_t subtreeFileSize;
        uint64_t size;
        std::string name;
    };

    /// <summary>
    /// Represents a diff that was applied to a filesystem node.
    /// </summary>
    struct FilesystemChangeInfo
    {
        FilesystemChangeKind diff;
        FilesystemNodeKind kind;
        uint64_t size;
        std::string name;
    };

    /// <summary>
    /// A node in a tree that describes a filesystem.
    /// </summary>
    using FilesystemTreeNodeRef = TreeNodeRef<FilesystemNodeInfo>;

    /// <summary>
    /// A node in a tree that describes diffs applied to a filesystem.
    /// </summary>
    using FilesystemChangeTreeNodeRef = TreeNodeRef<FilesystemChangeInfo>;

} // namespace Flense::Core