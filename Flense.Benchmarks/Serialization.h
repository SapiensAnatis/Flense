#pragma once

#include "Benchmarks.h"

#include <string>
#include <string_view>

namespace Flense::Benchmarks
{
    /// <summary>
    /// Serializes a benchmark result to JSON, for a child process to hand back to its parent.
    /// </summary>
    std::string SerializeResult(const BenchmarkResult& result);

    /// <summary>
    /// Parses a benchmark result previously produced by SerializeResult.
    /// </summary>
    /// <exception cref="nlohmann::json::exception">The JSON was malformed or missing a required field.</exception>
    BenchmarkResult DeserializeResult(std::string_view json);
} // namespace Flense::Benchmarks
