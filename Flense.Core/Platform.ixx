// Provides platform-specific functionality.
export module Flense.Core:Platform;

import std;

namespace Flense::Core
{
    /// <summary>
    /// Writes a line to the debugger output, if a debugger is attached.
    /// </summary>
    void DebugWriteLine(const std::string& message);

    /// <summary>
    /// Writes a line to the debugger output, if a debugger is attached.
    /// </summary>
    void DebugWriteLine(const char* nullTerminatedString);
} // namespace Flense::Core