#pragma once

namespace Flense::Core
{
    enum class FileKind
    {
        Unspecified = 0,
        File,
        Directory,
        Symlink,
        Other, // Socket / device / etc
    };
} // namespace Flense::Core