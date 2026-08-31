#include "pch.h"

#include "Benchmarks.h"
#include "FileByteStream.h"
#include "Progress.h"

#include <psapi.h>

import Flense.Core;

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <stdexcept>
#include <stop_token>
#include <unordered_set>
#include <vector>

namespace Flense::Benchmarks
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        constexpr size_t ReadChunkSize = 64 * 1024;
        constexpr size_t SlowestEntryCount = 10;

        /// <summary>
        /// Milliseconds elapsed since a start point.
        /// </summary>
        /// <param name="start">The start point.</param>
        /// <returns>The elapsed time in milliseconds.</returns>
        double ElapsedMs(const Clock::time_point start)
        {
            return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        }

        /// <summary>
        /// Reads the whole file through FileByteStream, doing nothing with the bytes. Establishes the
        /// floor that no amount of parser optimisation can beat.
        /// </summary>
        /// <param name="path">The image path.</param>
        /// <param name="reporter">The progress reporter.</param>
        /// <param name="passIndex">The index of the pass this stage belongs to.</param>
        /// <returns>The elapsed time in milliseconds.</returns>
        double RunIoPass(const std::filesystem::path& path, ProgressReporter& reporter, const int passIndex)
        {
            reporter.ReportPass(passIndex, "io");

            FileByteStream stream{path};
            std::vector<std::byte> buffer(ReadChunkSize);

            const auto start = Clock::now();

            while (stream.ReadSync(std::span(buffer)) != 0)
            {
            }

            return ElapsedMs(start);
        }

        struct ParsePassResult
        {
            double parseMs{0.0};
            double buildMs{0.0};
            size_t entryCount{0};
            std::vector<EntryTiming> entryTimings;
            Core::ImageDetails details;
        };

        /// <summary>
        /// The real workload: enumerate the archive, hand every entry to ImageParser, then Build().
        /// </summary>
        /// <param name="path">The image path.</param>
        /// <param name="collectEntryTimings">Whether to time each ProcessEntry call individually.</param>
        /// <param name="reporter">The progress reporter.</param>
        /// <param name="passIndex">The index of the pass this stage belongs to.</param>
        /// <returns>The parse and build timings, plus the resulting layers.</returns>
        /// <remarks>
        /// The parser and the returned layers are destroyed by the caller, after both timers have been
        /// stopped - tearing down a tree of a million shared_ptrs is not part of what we're measuring.
        /// </remarks>
        ParsePassResult RunParsePass(const std::filesystem::path& path, const bool collectEntryTimings,
                                     ProgressReporter& reporter, const int passIndex)
        {
            reporter.ReportPass(passIndex, "parse");

            ParsePassResult result;

            FileByteStream stream{path};
            Core::ImageParser parser;

            const auto parseStart = Clock::now();

            auto reader = Core::ArchiveReader::CreateFromStream(stream);

            while (auto entry = reader.Next(std::stop_token{}))
            {
                result.entryCount += 1;

                if (collectEntryTimings)
                {
                    // Captured before the call - ProcessEntry consumes the entry's body, and Pathname()
                    // is only valid until the next Next().
                    std::string pathname{entry->Pathname()};
                    const uint64_t size = entry->Size();

                    const auto entryStart = Clock::now();
                    parser.ProcessEntry(*entry, std::stop_token{});

                    result.entryTimings.emplace_back(std::move(pathname), size, ElapsedMs(entryStart));
                }
                else
                {
                    parser.ProcessEntry(*entry, std::stop_token{});
                }
            }

            result.parseMs = ElapsedMs(parseStart);

            const auto buildStart = Clock::now();
            result.details = parser.Build();
            result.buildMs = ElapsedMs(buildStart);

            return result;
        }

        /// <summary>
        /// Recursively counts nodes in a filesystem tree, tracking both the logical node count and the
        /// number of distinct allocations - the gap between the two is what structural sharing saves.
        /// </summary>
        /// <param name="node">The node to count from.</param>
        /// <param name="logicalCount">Accumulates the logical node count.</param>
        /// <param name="uniqueNodes">Accumulates the set of distinct node addresses.</param>
        void CountTreeNodes(const Core::FilesystemChangeTreeNodeRef& node, size_t& logicalCount,
                            std::unordered_set<const void*>& uniqueNodes)
        {
            if (!node)
            {
                return;
            }

            logicalCount += 1;
            uniqueNodes.insert(node.get());

            for (const auto& child : node->Children().values())
            {
                CountTreeNodes(child, logicalCount, uniqueNodes);
            }
        }

        /// <summary>
        /// The process's peak working set, as a rough guide to how much memory a parse costs.
        /// </summary>
        /// <returns>The peak working set in bytes, or 0 if it could not be read.</returns>
        uint64_t PeakWorkingSetBytes()
        {
            PROCESS_MEMORY_COUNTERS memoryCounters{};
            memoryCounters.cb = sizeof(memoryCounters);

            if (!GetProcessMemoryInfo(GetCurrentProcess(), &memoryCounters, sizeof(memoryCounters)))
            {
                return 0;
            }

            return memoryCounters.PeakWorkingSetSize;
        }

        /// <summary>
        /// Finds a phase's result in a result set that PhaseLayout() has already populated in display order.
        /// </summary>
        /// <param name="phases">The phase results.</param>
        /// <param name="phase">The phase to find.</param>
        /// <returns>A reference to the phase's result.</returns>
        PhaseResult& PhaseSlot(std::vector<PhaseResult>& phases, const Phase phase)
        {
            const auto it = std::ranges::find(phases, phase, &PhaseResult::phase);
            if (it == phases.end())
            {
                throw std::logic_error("PhaseSlot: phase missing from PhaseLayout().");
            }

            return *it;
        }

        /// <summary>
        /// The fixed, display-order set of phases a benchmark result reports - independent of whichever
        /// order RunBenchmark happens to measure them in.
        /// </summary>
        /// <returns>One empty PhaseResult per Phase, in display order.</returns>
        std::vector<PhaseResult> PhaseLayout()
        {
            std::vector<PhaseResult> phases;
            for (const Phase phase : {Phase::Io, Phase::ParseEntries, Phase::Build, Phase::EndToEnd})
            {
                phases.push_back(PhaseResult{.phase = phase, .samplesMs = {}});
            }

            return phases;
        }
    } // namespace

    std::string_view PhaseName(const Phase phase)
    {
        switch (phase)
        {
        case Phase::Io:
            return "io";
        case Phase::ParseEntries:
            return "parseEntries";
        case Phase::Build:
            return "build";
        case Phase::EndToEnd:
            return "endToEnd";
        }

        return "unknown";
    }

    Phase ParsePhaseName(const std::string_view name)
    {
        for (const Phase phase : {Phase::Io, Phase::ParseEntries, Phase::Build, Phase::EndToEnd})
        {
            if (PhaseName(phase) == name)
            {
                return phase;
            }
        }

        throw std::invalid_argument(std::format("Unknown phase name: {}", name));
    }

    double PhaseResult::MedianMs() const
    {
        if (samplesMs.empty())
        {
            return 0.0;
        }

        std::vector<double> sorted = samplesMs;
        std::ranges::sort(sorted);

        const size_t middle = sorted.size() / 2;

        // Averaging the two middle samples for an even count would invent a value that was never
        // measured; taking the upper of the two is the more conservative reading.
        return sorted[middle];
    }

    double PhaseResult::MinMs() const
    {
        const auto it = std::ranges::min_element(samplesMs);
        return it == samplesMs.end() ? 0.0 : *it;
    }

    double PhaseResult::MaxMs() const
    {
        const auto it = std::ranges::max_element(samplesMs);
        return it == samplesMs.end() ? 0.0 : *it;
    }

    BenchmarkResult RunBenchmark(const std::filesystem::path& imagePath, const int runs)
    {
        BenchmarkResult result;
        result.imagePath = imagePath;
        result.runs = runs;
        result.imageSizeBytes = std::filesystem::file_size(imagePath);
        result.phases = PhaseLayout();

        ProgressReporter reporter{imagePath.filename().string(), runs + 1};

        // Warm up the page cache and the allocator. An I/O pass alone would cache the file, but the
        // first parse also pays one-off costs (heap growth, page faults on first touch) that would
        // otherwise land entirely on run 1 and skew the max.
        RunIoPass(imagePath, reporter, 0);
        RunParsePass(imagePath, false, reporter, 0);

        Core::ImageDetails lastDetails;

        for (int run = 0; run < runs; run += 1)
        {
            const bool isFinalRun = run == runs - 1;
            const int passIndex = run + 1;

            PhaseSlot(result.phases, Phase::Io).samplesMs.push_back(RunIoPass(imagePath, reporter, passIndex));

            ParsePassResult pass = RunParsePass(imagePath, isFinalRun, reporter, passIndex);

            PhaseSlot(result.phases, Phase::ParseEntries).samplesMs.push_back(pass.parseMs);
            PhaseSlot(result.phases, Phase::Build).samplesMs.push_back(pass.buildMs);
            PhaseSlot(result.phases, Phase::EndToEnd).samplesMs.push_back(pass.parseMs + pass.buildMs);

            result.counters.entryCount = pass.entryCount;

            if (isFinalRun)
            {
                lastDetails = std::move(pass.details);

                result.slowestEntries = std::move(pass.entryTimings);
                std::ranges::sort(result.slowestEntries, std::ranges::greater{}, &EntryTiming::milliseconds);

                if (result.slowestEntries.size() > SlowestEntryCount)
                {
                    result.slowestEntries.resize(SlowestEntryCount);
                }
            }
        }

        result.counters.layerCount = lastDetails.layers.size();

        std::unordered_set<const void*> uniqueNodes;
        for (const Core::ImageLayer& layer : lastDetails.layers)
        {
            CountTreeNodes(layer.FilesystemChanges(), result.counters.treeNodeCount, uniqueNodes);
        }

        result.counters.uniqueTreeNodeCount = uniqueNodes.size();
        result.counters.peakWorkingSetBytes = PeakWorkingSetBytes();

        return result;
    }
} // namespace Flense::Benchmarks
