export module Flense.Core:Internal.FileHandle;

import std;

namespace Flense::Core
{
#if defined(_WIN32)
    using RawFileHandle = void*;
#else
#error "Unix coming soon (TM)"
#endif
} // namespace Flense::Core

export namespace Flense::Core
{
    struct HandleCloser
    {
        void operator()(RawFileHandle handle) noexcept;
    };

    using FileHandle = std::unique_ptr<std::remove_pointer_t<RawFileHandle>, HandleCloser>;
} // namespace Flense::Core