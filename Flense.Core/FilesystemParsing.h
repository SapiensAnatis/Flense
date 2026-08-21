#pragma once

#include "FilesystemTree.h"

#include <span>
#include <stop_token>

namespace Flense::Core
{
    class ArchiveReader;

    /// <summary>
    /// Populate a filesystem tree from a nested .tar archive inside an image layer.
    /// </summary>
    /// <param name="nestedTarReader"></param>
    /// <param name="stopToken">Handed to ArchiveReader::Next, so the walk can be interrupted part-way through a
    /// large entry. On cancellation the partial tree built so far is returned, so callers must discard it rather
    /// than treat it as a layer.</param>
    FilesystemChangeTreeNodeRef ParseLayerFilesystem(ArchiveReader* nestedTarReader, std::stop_token stopToken);

    /// <summary>
    /// Applies a diff tree on top of a base tree, producing the resulting merged filesystem state.
    /// </summary>
    /// <param name="base">The tree to patch. Every node in this tree is assumed to have a changeKind of
    /// FilesystemChangeKind::None.</param>
    /// <param name="diff">The tree of changes to apply on top of base, e.g. as produced by ParseLayerFilesystem.</param>
    /// <returns>A new tree representing base with diff applied.</returns>
    FilesystemChangeTreeNodeRef ApplyFilesystemChanges(const FilesystemChangeTreeNodeRef& base,
                                                         const FilesystemChangeTreeNodeRef& diff);

} // namespace Flense::Core