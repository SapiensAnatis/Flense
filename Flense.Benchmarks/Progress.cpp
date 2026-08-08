#include "pch.h"

#include "Progress.h"

#include <io.h>

#include <algorithm>
#include <format>
#include <iostream>
#include <stdio.h>

namespace Flense::Benchmarks
{
    namespace
    {
        /// <summary>
        /// Formats a duration as m:ss.
        /// </summary>
        /// <param name="duration">The duration.</param>
        /// <returns>A short human-readable string.</returns>
        std::string FormatClock(const std::chrono::steady_clock::duration duration)
        {
            const auto totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
            return std::format("{}:{:02}", totalSeconds / 60, totalSeconds % 60);
        }

        /// <summary>
        /// Whether stderr is a terminal, and so whether carriage-return redrawing makes sense.
        /// </summary>
        /// <returns>True if progress should be rendered.</returns>
        bool StderrIsTerminal()
        {
            return _isatty(_fileno(stderr)) != 0;
        }
    } // namespace

    void ClearConsole()
    {
        if (_isatty(_fileno(stdout)) == 0)
        {
            return;
        }

        const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD mode = 0;
        if (!GetConsoleMode(handle, &mode) || !SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            return;
        }

        // 2J clears the viewport, 3J the scrollback, H returns the cursor to the top left. The
        // scrollback matters here: without it the previous run stays scrolled just out of view.
        std::cout << "\x1b[2J\x1b[3J\x1b[H" << std::flush;

        // Enabling VT was a means to an end, not a preference to impose on the console.
        SetConsoleMode(handle, mode);
    }

    ProgressReporter::ProgressReporter(std::string imageLabel, const int totalPasses)
        : m_imageLabel(std::move(imageLabel)), m_totalPasses(std::max(totalPasses, 1)), m_enabled(StderrIsTerminal()),
          m_start(std::chrono::steady_clock::now())
    {
    }

    ProgressReporter::~ProgressReporter()
    {
        if (!m_enabled || m_lastLineLength == 0)
        {
            return;
        }

        std::cerr << '\r' << std::string(m_lastLineLength, ' ') << '\r' << std::flush;
    }

    void ProgressReporter::ReportPass(const int passIndex, const std::string_view label)
    {
        if (!m_enabled)
        {
            return;
        }

        // The elapsed time is from the start of this image's benchmark, and only advances when a pass
        // does. That is deliberate: the honest within-pass signal would be how far the parser has got
        // through the archive, but libarchive drains the file far ahead of the work it has actually
        // done, so a byte-based bar sits frozen at a misleadingly high number for seconds at a time.
        const std::string line =
            std::format("  {}  pass {}/{} ({})  elapsed {}", m_imageLabel, passIndex + 1, m_totalPasses, label,
                        FormatClock(std::chrono::steady_clock::now() - m_start));

        std::string padded = line;
        if (padded.size() < m_lastLineLength)
        {
            padded.append(m_lastLineLength - padded.size(), ' ');
        }

        m_lastLineLength = line.size();

        std::cerr << '\r' << padded << std::flush;
    }
} // namespace Flense::Benchmarks
