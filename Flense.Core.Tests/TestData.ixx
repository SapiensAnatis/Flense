export module Flense.Core.Tests.Support:TestData;

import std;

export namespace Flense::Core::Tests
{
    /// <summary>
    /// Resolves a path within the TestData directory the build copies next to the test executable.
    /// </summary>
    /// <param name="relativePath">The path within TestData, e.g. "ErrorCases/valid.tar".</param>
    std::filesystem::path TestDataPath(const std::filesystem::path& relativePath);
} // namespace Flense::Core::Tests
