#include "pch.h"

#include "Benchmarks.h"
#include "Progress.h"
#include "Reporting.h"
#include "Serialization.h"
#include "Subprocess.h"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using namespace Flense::Benchmarks;

    constexpr int Runs = 5;

    constexpr std::wstring_view TestDataDirectoryName = L"TestData";
    constexpr int MaxParentSearchDepth = 8;

    /// <summary>
    /// The flag that switches this executable into child mode: run one benchmark and print its
    /// result as JSON, rather than discovering images and printing human-readable reports.
    /// </summary>
    /// <remarks>
    /// Each image is benchmarked in its own child process because PeakWorkingSetSize is a process-wide
    /// high-water mark with no way to reset it - run everything in one process and only the first
    /// image's reading would mean anything.
    /// </remarks>
    constexpr std::wstring_view ChildModeFlag = L"--benchmark-child";

    /// <summary>
    /// Finds the TestData directory by walking up from the executable's own location.
    /// </summary>
    /// <remarks>
    /// Deliberately not based on the current working directory: that depends on how the benchmark was
    /// launched, and under the debugger it comes from a gitignored .vcxproj.user file.
    /// </remarks>
    /// <returns>The TestData directory, or nullopt if it could not be found.</returns>
    std::optional<std::filesystem::path> FindTestDataDirectory()
    {
        std::array<wchar_t, MAX_PATH> moduleFileName{};
        const DWORD length =
            GetModuleFileNameW(nullptr, moduleFileName.data(), static_cast<DWORD>(moduleFileName.size()));

        if (length == 0)
        {
            return std::nullopt;
        }

        std::filesystem::path directory = std::filesystem::path{moduleFileName.data()}.parent_path();

        for (int depth = 0; depth < MaxParentSearchDepth; depth += 1)
        {
            const std::filesystem::path candidate = directory / TestDataDirectoryName;
            if (std::filesystem::is_directory(candidate))
            {
                return candidate;
            }

            if (!directory.has_parent_path() || directory.parent_path() == directory)
            {
                break;
            }

            directory = directory.parent_path();
        }

        return std::nullopt;
    }

    /// <summary>
    /// Collects the .tar files in a directory, smallest first.
    /// </summary>
    /// <remarks>
    /// Smallest first so a quick sanity check comes back before the multi-gigabyte images are attempted.
    /// </remarks>
    /// <param name="directory">The directory to scan.</param>
    /// <returns>The discovered image paths, smallest first.</returns>
    std::vector<std::filesystem::path> DiscoverImages(const std::filesystem::path& directory)
    {
        std::vector<std::pair<uintmax_t, std::filesystem::path>> found;

        for (const auto& entry : std::filesystem::directory_iterator{directory})
        {
            if (entry.is_regular_file() && entry.path().extension() == L".tar")
            {
                found.emplace_back(entry.file_size(), entry.path());
            }
        }

        std::ranges::sort(found, {}, [](const auto& pair) { return pair.first; });

        std::vector<std::filesystem::path> paths;
        paths.reserve(found.size());

        for (auto& [size, path] : found)
        {
            paths.push_back(std::move(path));
        }

        return paths;
    }

    /// <summary>
    /// Runs one benchmark in-process and writes its result as JSON to stdout, for the orchestrator
    /// process to read back. Entered via ChildModeFlag on the command line.
    /// </summary>
    /// <param name="imagePath">The image to benchmark.</param>
    /// <param name="runs">The number of timed runs.</param>
    void RunChild(const std::filesystem::path& imagePath, const int runs)
    {
        const BenchmarkResult result = RunBenchmark(imagePath, runs);
        std::cout << SerializeResult(result);
    }

    /// <summary>
    /// Discovers the test images and benchmarks each one in its own child process, printing a
    /// human-readable report as each result comes back.
    /// </summary>
    /// <returns>The process exit code.</returns>
    int RunOrchestrator()
    {
        ClearConsole();

        const std::optional<std::filesystem::path> testData = FindTestDataDirectory();
        if (!testData)
        {
            std::cerr << "No TestData directory found above the executable.\n"
                         "Run TestData\\GenerateTestData.ps1 to fetch the images.\n";
            return 1;
        }

        const std::vector<std::filesystem::path> imagePaths = DiscoverImages(*testData);
        if (imagePaths.empty())
        {
            std::cerr << std::format("No .tar files in '{}'. Run TestData\\GenerateTestData.ps1 first.\n",
                                     testData->string());
            return 1;
        }

        for (const std::filesystem::path& imagePath : imagePaths)
        {
            std::cout << std::format("Benchmarking {}...\n", imagePath.filename().string()) << std::flush;

            const std::string json = RunSelfCapturingStdout(
                {std::wstring{ChildModeFlag}, imagePath.wstring(), std::to_wstring(Runs)});

            PrintReport(DeserializeResult(json));
            std::cout << std::flush;
        }

        return 0;
    }
} // namespace

int wmain(const int argc, wchar_t* argv[])
{
    try
    {
        if (argc == 4 && argv[1] == ChildModeFlag)
        {
            RunChild(argv[2], std::stoi(std::wstring{argv[3]}));
            return 0;
        }

        return RunOrchestrator();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "\nError: " << exception.what() << '\n';
        return 1;
    }
}
