#pragma once

#include "FilesystemTree.h"

#include <span>

namespace Flense::Core
{
    class ArchiveReader;

    /// <summary>
    /// Populate a filesystem tree from a nested .tar archive inside an image layer.
    /// </summary>
    /// <param name="nestedTarReader"></param>
    FilesystemChangeTreeNodeRef ParseLayerFilesystem(ArchiveReader* nestedTarReader);

} // namespace Flense::Core