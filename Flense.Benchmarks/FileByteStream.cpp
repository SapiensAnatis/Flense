#include "pch.h"

#include "FileByteStream.h"

#include "ArchiveReader.h"

#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>

namespace Flense::Benchmarks
{
    namespace
    {
        [[noreturn]] void ThrowLastError(const std::filesystem::path& path, const char* operation)
        {
            throw std::runtime_error(
                std::format("{} failed for '{}' (GetLastError = {})", operation, path.string(), GetLastError()));
        }
    } // namespace

    FileByteStream::FileByteStream(const std::filesystem::path& path) : m_handle(INVALID_HANDLE_VALUE)
    {
        // FILE_FLAG_SEQUENTIAL_SCAN is a hint that matches how the archive reader actually walks the
        // file, and keeps the page cache behaviour representative of the real app's access pattern.
        m_handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

        if (m_handle == INVALID_HANDLE_VALUE)
        {
            ThrowLastError(path, "CreateFileW");
        }

        LARGE_INTEGER size{};
        if (!GetFileSizeEx(m_handle, &size))
        {
            const DWORD error = GetLastError();
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
            SetLastError(error);
            ThrowLastError(path, "GetFileSizeEx");
        }

        m_size = static_cast<uint64_t>(size.QuadPart);
    }

    FileByteStream::~FileByteStream()
    {
        if (m_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_handle);
        }
    }

    size_t FileByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        const DWORD toRead = static_cast<DWORD>(std::min<size_t>(buffer.size(), std::numeric_limits<DWORD>::max()));

        DWORD bytesRead = 0;
        if (!ReadFile(m_handle, buffer.data(), toRead, &bytesRead, nullptr))
        {
            return 0;
        }

        m_position += bytesRead;
        return bytesRead;
    }

    int64_t FileByteStream::Skip(const int64_t request)
    {
        if (request <= 0)
        {
            return 0;
        }

        const uint64_t remaining = m_size - m_position;
        const int64_t toSkip = static_cast<int64_t>(std::min<uint64_t>(static_cast<uint64_t>(request), remaining));

        if (toSkip == 0)
        {
            return 0;
        }

        LARGE_INTEGER distance{};
        distance.QuadPart = toSkip;

        if (!SetFilePointerEx(m_handle, distance, nullptr, FILE_CURRENT))
        {
            return 0;
        }

        m_position += static_cast<uint64_t>(toSkip);
        return toSkip;
    }

    uint64_t FileByteStream::Size() const
    {
        return m_size;
    }

    uint64_t FileByteStream::Position() const
    {
        return m_position;
    }

    static_assert(Core::ByteStream<FileByteStream>);
} // namespace Flense::Benchmarks
