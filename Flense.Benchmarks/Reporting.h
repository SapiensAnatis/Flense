#pragma once

#include "Benchmarks.h"

namespace Flense::Benchmarks
{
    /// <summary>
    /// Writes a human-readable report for one image to stdout: the phase table, the slowest
    /// individual entries, and the counters describing the image's shape.
    /// </summary>
    /// <param name="result">The benchmark result.</param>
    void PrintReport(const BenchmarkResult& result);
} // namespace Flense::Benchmarks
