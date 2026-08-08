#include "pch.h"

#include "Reporting.h"

#include <array>
#include <format>
#include <iostream>
#include <string>

namespace Flense::Benchmarks
{
    namespace
    {
        constexpr int PhaseColumnWidth = 26;
        constexpr int NumberColumnWidth = 11;

        /// <summary>
        /// Formats a byte count using binary units.
        /// </summary>
        /// <param name="bytes">The byte count.</param>
        /// <returns>A human-readable string.</returns>
        std::string FormatBytes(const uint64_t bytes)
        {
            constexpr std::array<std::string_view, 5> units{"B", "KiB", "MiB", "GiB", "TiB"};

            double value = static_cast<double>(bytes);
            size_t unit = 0;

            while (value >= 1024.0 && unit + 1 < units.size())
            {
                value /= 1024.0;
                unit += 1;
            }

            if (unit == 0)
            {
                return std::format("{} B", bytes);
            }

            return std::format("{:.1f} {}", value, units[unit]);
        }

        /// <summary>
        /// Formats a duration, switching to seconds once it stops being readable in milliseconds.
        /// </summary>
        /// <param name="milliseconds">The duration in milliseconds.</param>
        /// <returns>A human-readable string.</returns>
        std::string FormatDuration(const double milliseconds)
        {
            if (milliseconds >= 1000.0)
            {
                return std::format("{:.2f} s", milliseconds / 1000.0);
            }

            return std::format("{:.1f} ms", milliseconds);
        }

        /// <summary>
        /// Formats an integer with thousands separators.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>A grouped string, e.g. "1,284,301".</returns>
        std::string GroupDigits(const uint64_t value)
        {
            std::string digits = std::to_string(value);
            std::string grouped;
            grouped.reserve(digits.size() + digits.size() / 3);

            for (size_t i = 0; i < digits.size(); i += 1)
            {
                if (i != 0 && (digits.size() - i) % 3 == 0)
                {
                    grouped.push_back(',');
                }

                grouped.push_back(digits[i]);
            }

            return grouped;
        }

        /// <summary>
        /// Formats a throughput figure, guarding against a duration too small to divide by.
        /// </summary>
        /// <param name="bytes">The number of bytes processed.</param>
        /// <param name="milliseconds">The time taken.</param>
        /// <returns>A human-readable rate, or "-" if it is not meaningful.</returns>
        std::string FormatThroughput(const uint64_t bytes, const double milliseconds)
        {
            if (milliseconds <= 0.0 || bytes == 0)
            {
                return "-";
            }

            const double mibPerSecond = (static_cast<double>(bytes) / (1024.0 * 1024.0)) / (milliseconds / 1000.0);
            return std::format("{:.0f} MiB/s", mibPerSecond);
        }

        /// <summary>
        /// Writes a horizontal rule of the given width.
        /// </summary>
        /// <param name="width">The rule's width in characters.</param>
        void PrintRule(const size_t width)
        {
            std::cout << std::string(width, '-') << '\n';
        }

        /// <summary>
        /// Writes the phase timing table.
        /// </summary>
        /// <param name="result">The benchmark result.</param>
        void PrintPhaseTable(const BenchmarkResult& result)
        {
            constexpr size_t tableWidth = PhaseColumnWidth + (NumberColumnWidth * 3);

            std::cout << std::format("{:<{}}{:>{}}{:>{}}{:>{}}\n", "Phase", PhaseColumnWidth, "median",
                                     NumberColumnWidth, "min", NumberColumnWidth, "max", NumberColumnWidth);
            PrintRule(tableWidth);

            for (const PhaseResult& phase : result.phases)
            {
                // endToEnd is the sum of the two phases above it, so setting it apart keeps the table
                // from reading as though its time were additional.
                if (phase.phase == Phase::EndToEnd)
                {
                    PrintRule(tableWidth);
                }

                std::cout << std::format("{:<{}}{:>{}}{:>{}}{:>{}}\n", PhaseName(phase.phase), PhaseColumnWidth,
                                         FormatDuration(phase.MedianMs()), NumberColumnWidth,
                                         FormatDuration(phase.MinMs()), NumberColumnWidth,
                                         FormatDuration(phase.MaxMs()), NumberColumnWidth);
            }
        }

        /// <summary>
        /// Writes the slowest individual entries.
        /// </summary>
        /// <param name="result">The benchmark result.</param>
        void PrintSlowestEntries(const BenchmarkResult& result)
        {
            if (result.slowestEntries.empty())
            {
                return;
            }

            std::cout << "\nSlowest entries (final run)\n";

            if (result.workerCount != 1)
            {
                // With workers, ProcessEntry only copies a layer's compressed bytes out of the archive -
                // the inflate happens elsewhere - so these times measure the hand-off, not the work.
                std::cout << "(hand-off time only; layer decompression is not included)\n";
            }

            std::cout << std::format("{:>12}{:>13}{:>14}  {}\n", "time", "size", "throughput", "path");
            PrintRule(12 + 13 + 14 + 2 + 40);

            for (const EntryTiming& entry : result.slowestEntries)
            {
                std::cout << std::format("{:>12}{:>13}{:>14}  {}\n", FormatDuration(entry.milliseconds),
                                         FormatBytes(entry.size), FormatThroughput(entry.size, entry.milliseconds),
                                         entry.pathname);
            }
        }

        /// <summary>
        /// Writes the counters describing the parsed image's shape.
        /// </summary>
        /// <param name="result">The benchmark result.</param>
        void PrintCounters(const BenchmarkResult& result)
        {
            const Counters& counters = result.counters;

            std::cout << '\n';
            std::cout << std::format("Entries: {}   Layers: {}\n", GroupDigits(counters.entryCount),
                                     GroupDigits(counters.layerCount));

            if (counters.treeNodeCount > 0)
            {
                const double sharing = 100.0 - ((static_cast<double>(counters.uniqueTreeNodeCount) /
                                                 static_cast<double>(counters.treeNodeCount)) *
                                                100.0);

                std::cout << std::format("Tree nodes: {} logical, {} distinct ({:.1f}% shared)\n",
                                         GroupDigits(counters.treeNodeCount),
                                         GroupDigits(counters.uniqueTreeNodeCount), sharing);
            }

            if (result.workerCount != 1)
            {
                std::cout << std::format("Read-ahead stall: {}\n", FormatDuration(counters.readAheadStallMs));
            }

            std::cout << std::format("Peak working set: {}\n", FormatBytes(counters.peakWorkingSetBytes));
        }
    } // namespace

    void PrintReport(const BenchmarkResult& result)
    {
        std::cout << std::format("\nImage: {} ({})\n", result.imagePath.filename().string(),
                                 FormatBytes(result.imageSizeBytes));
        std::cout << std::format("Runs : {} (1 warmup, warm page cache)\n", result.runs);
        std::cout << std::format("Layer workers: {}\n\n", result.workerCount);

        PrintPhaseTable(result);
        PrintSlowestEntries(result);
        PrintCounters(result);
    }
} // namespace Flense::Benchmarks
