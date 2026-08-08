#pragma once

#include <chrono>
#include <string>
#include <string_view>

namespace Flense::Benchmarks
{
    /// <summary>
    /// Clears the console, scrollback included, so that each run starts from a blank screen rather
    /// than being read against the previous run's output.
    /// </summary>
    /// <remarks>
    /// Does nothing when stdout is redirected - escape sequences in a captured report would be worse
    /// than the problem they solve.
    /// </remarks>
    void ClearConsole();

    /// <summary>
    /// Reports which pass a benchmark has reached, as a single line rewritten in place, so that a
    /// long run over a large image gives some sign of life.
    /// </summary>
    /// <remarks>
    /// Writes to stderr, leaving stdout as a clean report that can be redirected to a file, and
    /// silently disables itself when stderr is not a terminal.
    /// </remarks>
    class ProgressReporter
    {
      public:
        /// <summary>
        /// Starts reporting.
        /// </summary>
        /// <param name="imageLabel">The image name to show.</param>
        /// <param name="totalPasses">The number of passes to be run, warmups included.</param>
        ProgressReporter(std::string imageLabel, int totalPasses);

        /// <summary>
        /// Erases the status line, leaving the terminal clean for the report.
        /// </summary>
        ~ProgressReporter();

        ProgressReporter(const ProgressReporter&) = delete;
        ProgressReporter& operator=(const ProgressReporter&) = delete;

        /// <summary>
        /// Reports that a pass has started.
        /// </summary>
        /// <param name="passIndex">The zero-based index of the pass.</param>
        /// <param name="label">A short description of the stage, e.g. "parse".</param>
        void ReportPass(int passIndex, std::string_view label);

      private:
        std::string m_imageLabel;
        int m_totalPasses;
        bool m_enabled;
        std::chrono::steady_clock::time_point m_start;
        size_t m_lastLineLength{0};
    };
} // namespace Flense::Benchmarks
