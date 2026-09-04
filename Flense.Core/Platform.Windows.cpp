module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

module Flense.Core;

import :Platform;

import std;

namespace Flense::Core
{
    void DebugWriteLine(const std::string& message)
    {
        DebugWriteLine(message.c_str());
    }

    void DebugWriteLine(const char* nullTerminatedString)
    {
        OutputDebugStringA(nullTerminatedString);
        OutputDebugStringA("\n");
    }

} // namespace Flense::Core