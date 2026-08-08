#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Flense::Benchmarks
{
    /// <summary>
    /// The distinct stages of reading an image, each measured independently so that a slow parse can
    /// be attributed to a cause rather than just observed.
    /// </summary>
    enum class Phase
    {
        /// <summary>Reading the file's bytes and nothing else - the floor imposed by I/O.</summary>
        Io,

        /// <summary>The real parse loop: ImageParser::ProcessEntry on every entry.</summary>
        ParseEntries,

        /// <summary>ImageParser::Build - folding layer diffs into per-layer snapshots.</summary>
        Build,

        /// <summary>ParseEntries plus Build, i.e. what the app actually waits for.</summary>
        EndToEnd,
    };

    /// <summary>
    /// Gets the stable, machine-readable name of a phase, used in both the console table and the JSON.
    /// </summary>
    /// <param name="phase">The phase.</param>
    /// <returns>The phase's name.</returns>
    std::string_view PhaseName(Phase phase);

    /// <summary>
    /// The timings collected for one phase across all runs.
    /// </summary>
    struct PhaseResult
    {
        Phase phase;

        /// <summary>Per-run durations in milliseconds, in the order they were measured.</summary>
        std::vector<double> samplesMs;

        [[nodiscard]] double MedianMs() const;
        [[nodiscard]] double MinMs() const;
        [[nodiscard]] double MaxMs() const;
    };

    /// <summary>
    /// The time ProcessEntry spent on one individual archive entry.
    /// </summary>
    struct EntryTiming
    {
        std::string pathname;
        uint64_t size{0};
        double milliseconds{0.0};
    };

    /// <summary>
    /// Counts describing the shape of the parsed image, gathered once outside the timed runs.
    /// </summary>
    struct Counters
    {
        size_t entryCount{0};
        size_t layerCount{0};

        /// <summary>Total nodes reachable by walking every layer's tree - the logical tree size.</summary>
        size_t treeNodeCount{0};

        /// <summary>Distinct nodes by address across all layers - how much structural sharing achieves.</summary>
        size_t uniqueTreeNodeCount{0};

        /// <summary>
        /// Time the archive walk spent blocked waiting for read-ahead memory, from the final run.
        /// Zero means the budget was never the constraint; a large value means raising it would help.
        /// </summary>
        double readAheadStallMs{0.0};

        uint64_t peakWorkingSetBytes{0};
    };

    /// <summary>
    /// Everything measured for one image.
    /// </summary>
    struct BenchmarkResult
    {
        std::filesystem::path imagePath;
        uint64_t imageSizeBytes{0};
        int runs{0};

        /// <summary>The number of layer-decompression threads the parser was configured with.</summary>
        size_t workerCount{1};

        std::vector<PhaseResult> phases;
        Counters counters;

        /// <summary>The slowest individual entries from the final timed run, descending.</summary>
        std::vector<EntryTiming> slowestEntries;
    };

    /// <summary>
    /// Runs the benchmark for a single image: one warmup pass to bring the file into the page cache,
    /// then the requested number of timed runs.
    /// </summary>
    /// <param name="imagePath">The image to benchmark.</param>
    /// <param name="runs">The number of timed runs.</param>
    /// <param name="workerCount">The parser's layer-decompression thread count. 1 runs everything on
    /// the archive-walking thread, which is the baseline the parallel results are compared against.</param>
    /// <returns>The collected results.</returns>
    /// <exception cref="std::runtime_error">The image could not be opened.</exception>
    BenchmarkResult RunBenchmark(const std::filesystem::path& imagePath, int runs, size_t workerCount);

    /// <summary>
    /// Parses an image serially and again in parallel, and checks the two produce identical layers.
    /// </summary>
    /// <remarks>
    /// A parallel parse that is merely fast is worthless, and a corrupted filesystem tree is not
    /// something the timings would ever reveal - so this runs before any of them.
    /// </remarks>
    /// <param name="imagePath">The image to check.</param>
    /// <param name="workerCount">The worker count to compare the serial parse against.</param>
    /// <returns>An empty string if the two agree, otherwise a description of the first difference.</returns>
    std::string VerifyParallelMatchesSerial(const std::filesystem::path& imagePath, size_t workerCount);
} // namespace Flense::Benchmarks
