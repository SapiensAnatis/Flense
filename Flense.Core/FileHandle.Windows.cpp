module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

module Flense.Core.Shared:FileHandle.Windows;

import :FileHandle;

namespace Flense::Core
{
    void HandleCloser::operator()(RawFileHandle handle) noexcept
    {
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
        }
    }
} // namespace Flense::Core