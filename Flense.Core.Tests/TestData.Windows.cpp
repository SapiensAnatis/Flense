module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

module Flense.Core.Tests.Support;

import std;

namespace Flense::Core::Tests
{
    std::filesystem::path TestDataPath(const std::filesystem::path& relativePath)
    {
        std::wstring modulePath(MAX_PATH, L'\0');

        while (true)
        {
            const DWORD length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
            if (length == 0)
            {
                throw std::runtime_error(std::format("GetModuleFileNameW failed (GetLastError = {})", GetLastError()));
            }

            if (length < modulePath.size())
            {
                modulePath.resize(length);
                break;
            }

            modulePath.resize(modulePath.size() * 2);
        }

        return (std::filesystem::path{modulePath}.parent_path() / "TestData" / relativePath).make_preferred();
    }
} // namespace Flense::Core::Tests
