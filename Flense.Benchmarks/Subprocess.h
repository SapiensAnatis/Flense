#pragma once

#include <string>
#include <vector>

namespace Flense::Benchmarks
{
    /// <summary>
    /// Runs the current executable as a child process with the given arguments, inheriting stdin and
    /// stderr - so progress reporting still displays live - but capturing stdout, which is returned
    /// once the child exits.
    /// </summary>
    /// <param name="args">The arguments to pass, not including the executable's own path.</param>
    /// <returns>Everything the child wrote to stdout.</returns>
    /// <exception cref="std::runtime_error">The child could not be started, or exited non-zero.</exception>
    std::string RunSelfCapturingStdout(const std::vector<std::wstring>& args);
} // namespace Flense::Benchmarks
