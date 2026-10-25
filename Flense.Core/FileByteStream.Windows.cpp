module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

module Flense.Core:FileByteStream.Windows;

import :FileByteStream;

import std;

namespace Flense::Core
{
    namespace
    {
        [[noreturn]] void ThrowLastError(const char* operation, const std::filesystem::path& path)
        {
            throw std::filesystem::filesystem_error(
                operation, path, std::error_code(static_cast<int>(GetLastError()), std::system_category()));
        }
    } // namespace

    void FileByteStream::HandleCloser::operator()(void* handle) const noexcept
    {
        CloseHandle(handle);
    }

    FileByteStream::FileByteStream(const std::filesystem::path& path) : m_path(path)
    {
        HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

        // Only handed to m_handle once known to be valid, as INVALID_HANDLE_VALUE isn't nullptr.
        if (handle == INVALID_HANDLE_VALUE)
        {
            ThrowLastError("CreateFileW", path);
        }

        m_handle.reset(handle);

        LARGE_INTEGER size{};
        if (GetFileSizeEx(m_handle.get(), &size) == FALSE)
        {
            ThrowLastError("GetFileSizeEx", path);
        }

        m_size = static_cast<std::uint64_t>(size.QuadPart);
    }

    std::size_t FileByteStream::ReadChunkAt(const std::uint64_t offset)
    {
        OVERLAPPED overlapped{};
        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) - OVERLAPPED keeps its offset in a union
        overlapped.Offset = static_cast<DWORD>(offset);
        overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32U);
        // NOLINTEND(cppcoreguidelines-pro-type-union-access)

        DWORD bytesRead = 0;
        if (ReadFile(m_handle.get(), m_buffer.data(), static_cast<DWORD>(m_buffer.size()), &bytesRead, &overlapped) ==
            FALSE)
        {
            if (GetLastError() == ERROR_HANDLE_EOF)
            {
                return 0;
            }

            ThrowLastError("ReadFile", m_path);
        }

        return bytesRead;
    }
} // namespace Flense::Core
