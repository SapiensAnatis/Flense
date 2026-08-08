#include "pch.h"

#include "Benchmarks.h"
#include "FileByteStream.h"
#include "Progress.h"

#include "ArchiveReader.h"
#include "FilesystemTree.h"
#include "ImageLayer.h"
#include "ImageParser.h"

#include <psapi.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <format>
#include <stop_token>
#include <string>
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
            double readAheadStallMs{0.0};
            size_t entryCount{0};
            std::vector<EntryTiming> entryTimings;
            std::vector<Core::ImageLayer> layers;
        };

        /// <summary>
        /// The real workload: enumerate the archive, hand every entry to ImageParser, then Build().
        /// </summary>
        /// <param name="path">The image path.</param>
        /// <param name="workerCount">The parser's layer-decompression thread count.</param>
        /// <param name="collectEntryTimings">Whether to time each ProcessEntry call individually.</param>
        /// <param name="reporter">The progress reporter.</param>
        /// <param name="passIndex">The index of the pass this stage belongs to.</param>
        /// <returns>The parse and build timings, plus the resulting layers.</returns>
        /// <remarks>
        /// The parser and the returned layers are destroyed by the caller, after both timers have been
        /// stopped - tearing down a tree of a million shared_ptrs is not part of what we're measuring.
        ///
        /// With more than one worker the parse/build split stops being a split of the work: ProcessEntry
        /// returns as soon as a layer's compressed bytes have been copied out, and Build() is where the
        /// wait for the decompressions actually lands. Only endToEnd is comparable across worker counts.
        /// </remarks>
        ParsePassResult RunParsePass(const std::filesystem::path& path, const size_t workerCount,
                                     const bool collectEntryTimings, ProgressReporter& reporter, const int passIndex)
        {
            reporter.ReportPass(passIndex, "parse");

            ParsePassResult result;

            FileByteStream stream{path};
            Core::ImageParser parser{Core::ImageParserOptions{.workerCount = workerCount}};

            const auto parseStart = Clock::now();

            auto reader = Core::ArchiveReader::CreateFromStream(stream, std::stop_token{});

            while (auto entry = reader.Next())
            {
                result.entryCount += 1;

                if (collectEntryTimings)
                {
                    // Captured before the call - ProcessEntry consumes the entry's body, and Pathname()
                    // is only valid until the next Next().
                    std::string pathname{entry->Pathname()};
                    const uint64_t size = entry->Size();

                    const auto entryStart = Clock::now();
                    parser.ProcessEntry(*entry);

                    result.entryTimings.emplace_back(std::move(pathname), size, ElapsedMs(entryStart));
                }
                else
                {
                    parser.ProcessEntry(*entry);
                }
            }

            result.parseMs = ElapsedMs(parseStart);

            const auto buildStart = Clock::now();
            result.layers = parser.Build();
            result.buildMs = ElapsedMs(buildStart);

            result.readAheadStallMs =
                std::chrono::duration<double, std::milli>(parser.ReadAheadStallTime()).count();

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
        /// Compares two filesystem trees by value, describing the first place they diverge.
        /// </summary>
        /// <param name="left">The first tree.</param>
        /// <param name="right">The second tree.</param>
        /// <param name="path">The path walked to reach these two nodes, for the message.</param>
        /// <returns>An empty string if the trees are equal, otherwise a description of the difference.</returns>
        std::string DescribeTreeDifference(const Core::FilesystemChangeTreeNodeRef& left,
                                           const Core::FilesystemChangeTreeNodeRef& right, const std::string& path)
        {
            if (!left || !right)
            {
                return left == right ? std::string{} : std::format("'{}' present on only one side", path);
            }

            if (!(left->Data() == right->Data()))
            {
                return std::format("'{}' has differing info", path);
            }

            const auto& leftChildren = left->Children();
            const auto& rightChildren = right->Children();

            if (leftChildren.size() != rightChildren.size())
            {
                return std::format("'{}' has {} children vs {}", path, leftChildren.size(), rightChildren.size());
            }

            for (const auto& [name, leftChild] : leftChildren)
            {
                const auto it = rightChildren.find(name);
                if (it == rightChildren.end())
                {
                    return std::format("'{}/{}' missing on one side", path, name);
                }

                if (std::string difference = DescribeTreeDifference(leftChild, it->second, path + "/" + name);
                    !difference.empty())
                {
                    return difference;
                }
            }

            return {};
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
        /// Finds a phase's result in a result set, adding it if it isn't there yet.
        /// </summary>
        /// <param name="phases">The phase results.</param>
        /// <param name="phase">The phase to find.</param>
        /// <returns>A reference to the phase's result.</returns>
        PhaseResult& PhaseSlot(std::vector<PhaseResult>& phases, const Phase phase)
        {
            const auto it = std::ranges::find(phases, phase, &PhaseResult::phase);
            if (it != phases.end())
            {
                return *it;
            }

            phases.push_back(PhaseResult{.phase = phase, .samplesMs = {}});
            return phases.back();
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

    BenchmarkResult RunBenchmark(const std::filesystem::path& imagePath, const int runs, const size_t workerCount)
    {
        BenchmarkResult result;
        result.imagePath = imagePath;
        result.runs = runs;
        result.workerCount = workerCount;
        result.imageSizeBytes = std::filesystem::file_size(imagePath);

        ProgressReporter reporter{imagePath.filename().string(), runs + 1};

        // Warm up the page cache and the allocator. An I/O pass alone would cache the file, but the
        // first parse also pays one-off costs (heap growth, page faults on first touch) that would
        // otherwise land entirely on run 1 and skew the max.
        RunIoPass(imagePath, reporter, 0);
        RunParsePass(imagePath, workerCount, false, reporter, 0);

        std::vector<Core::ImageLayer> lastLayers;

        for (int run = 0; run < runs; run += 1)
        {
            const bool isFinalRun = run == runs - 1;
            const int passIndex = run + 1;

            PhaseSlot(result.phases, Phase::Io).samplesMs.push_back(RunIoPass(imagePath, reporter, passIndex));

            ParsePassResult pass = RunParsePass(imagePath, workerCount, isFinalRun, reporter, passIndex);

            PhaseSlot(result.phases, Phase::ParseEntries).samplesMs.push_back(pass.parseMs);
            PhaseSlot(result.phases, Phase::Build).samplesMs.push_back(pass.buildMs);
            PhaseSlot(result.phases, Phase::EndToEnd).samplesMs.push_back(pass.parseMs + pass.buildMs);

            result.counters.entryCount = pass.entryCount;
            result.counters.readAheadStallMs = pass.readAheadStallMs;

            if (isFinalRun)
            {
                lastLayers = std::move(pass.layers);

                result.slowestEntries = std::move(pass.entryTimings);
                std::ranges::sort(result.slowestEntries, std::ranges::greater{}, &EntryTiming::milliseconds);

                if (result.slowestEntries.size() > SlowestEntryCount)
                {
                    result.slowestEntries.resize(SlowestEntryCount);
                }
            }
        }

        result.counters.layerCount = lastLayers.size();

        std::unordered_set<const void*> uniqueNodes;
        for (const Core::ImageLayer& layer : lastLayers)
        {
            CountTreeNodes(layer.FilesystemChanges(), result.counters.treeNodeCount, uniqueNodes);
        }

        result.counters.uniqueTreeNodeCount = uniqueNodes.size();
        result.counters.peakWorkingSetBytes = PeakWorkingSetBytes();

        return result;
    }

    std::string VerifyParallelMatchesSerial(const std::filesystem::path& imagePath, const size_t workerCount)
    {
        ProgressReporter reporter{imagePath.filename().string(), 2};

        const std::vector<Core::ImageLayer> serial = RunParsePass(imagePath, 1, false, reporter, 0).layers;
        const std::vector<Core::ImageLayer> parallel = RunParsePass(imagePath, workerCount, false, reporter, 1).layers;

        if (serial.size() != parallel.size())
        {
            return std::format("layer count differs: {} serial vs {} parallel", serial.size(), parallel.size());
        }

        for (size_t i = 0; i < serial.size(); i += 1)
        {
            if (serial[i].Command() != parallel[i].Command())
            {
                return std::format("layer {} command differs", i);
            }

            if (const std::string difference =
                    DescribeTreeDifference(serial[i].FilesystemChanges(), parallel[i].FilesystemChanges(), "");
                !difference.empty())
            {
                return std::format("layer {}: {}", i, difference);
            }
        }

        return {};
    }
} // namespace Flense::Benchmarks
