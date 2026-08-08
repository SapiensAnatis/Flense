#include "pch.h"

#include "Benchmarks.h"
#include "Progress.h"
#include "Reporting.h"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
    using namespace Flense::Benchmarks;

    constexpr int Runs = 5;

    constexpr std::wstring_view TestDataDirectoryName = L"TestData";
    constexpr int MaxParentSearchDepth = 8;

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
    /// Ordering earns its keep twice over: a quick sanity check comes back before the multi-gigabyte
    /// images are attempted, and the peak working set counter - which is process-wide, with no way to
    /// reset it - is only meaningful for whichever image runs first.
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
    /// The parser worker counts to sweep, ascending.
    /// </summary>
    /// <remarks>
    /// 1 is the serial baseline everything else is judged against. The rest are powers of two up to
    /// the hardware thread count, which is enough to show where the speedup curve flattens without
    /// multiplying the runtime of an already slow benchmark.
    /// </remarks>
    /// <returns>The worker counts to benchmark.</returns>
    std::vector<size_t> WorkerCountSweep()
    {
        const size_t hardwareThreads = std::max<size_t>(1, std::thread::hardware_concurrency());

        std::vector<size_t> counts{1};

        for (size_t count = 2; count < hardwareThreads; count *= 2)
        {
            counts.push_back(count);
        }

        if (hardwareThreads > 1)
        {
            counts.push_back(hardwareThreads);
        }

        return counts;
    }
} // namespace

int main()
{
    try
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

        const std::vector<size_t> workerCounts = WorkerCountSweep();

        for (const std::filesystem::path& imagePath : imagePaths)
        {
            const std::string imageName = imagePath.filename().string();

            // A faster parse that produces a different tree is not a faster parse. Check that before
            // spending several minutes measuring it.
            std::cout << std::format("Verifying {} parses identically in parallel...\n", imageName) << std::flush;

            if (const std::string difference = VerifyParallelMatchesSerial(imagePath, workerCounts.back());
                !difference.empty())
            {
                std::cerr << std::format("\nParallel parse of {} does not match the serial parse: {}\n", imageName,
                                         difference);
                return 1;
            }

            for (const size_t workerCount : workerCounts)
            {
                std::cout << std::format("Benchmarking {} ({} worker{})...\n", imageName, workerCount,
                                         workerCount == 1 ? "" : "s")
                          << std::flush;

                const BenchmarkResult result = RunBenchmark(imagePath, Runs, workerCount);

                PrintReport(result);
                std::cout << std::flush;
            }
        }

        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "\nError: " << exception.what() << '\n';
        return 1;
    }
}
