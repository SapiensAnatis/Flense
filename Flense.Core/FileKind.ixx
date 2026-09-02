export module Flense.Core:FileKind;

import std;

export namespace Flense::Core
{
    enum class FileKind : std::uint8_t
    {
        Unspecified = 0,
        File,
        Directory,
        Symlink,
        Other, // Socket / device / etc
    };
} // namespace Flense::Core
