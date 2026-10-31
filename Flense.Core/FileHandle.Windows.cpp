module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>

module Flense.Core:Internal.FileHandle.Windows;

import :Internal.FileHandle;

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