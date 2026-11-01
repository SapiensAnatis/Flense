module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

module Flense.Core.DockerClient:DockerTransport.Windows;

import Flense.Core.Http;
import Flense.Core.Shared;

import :DockerTransport;

import std;

namespace Flense::Core
{
    namespace
    {
        [[noreturn]] void ThrowLastError(const char* operation)
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
        }

        FileHandle GetDockerPipeHandle()
        {
            // TODO: Respect DOCKER_HOST so that users on a non-default context reach the right daemon.
            // TODO: Retry with WaitNamedPipeW when this fails with ERROR_PIPE_BUSY.
            const std::wstring pipeName = LR"(\\.\pipe\docker_engine)";
            HANDLE pipeHandle =
                CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (pipeHandle == INVALID_HANDLE_VALUE)
            {
                ThrowLastError("Failed to open Docker named pipe");
            }

            return FileHandle(pipeHandle);
        }

    } // namespace

    std::size_t DockerConnection::ReadSync(const std::span<std::byte> buffer)
    {
        DWORD bytesRead = 0;
        const DWORD toRead =
            static_cast<DWORD>(std::min<std::size_t>(buffer.size(), std::numeric_limits<DWORD>::max()));
        if (ReadFile(m_handle.get(), buffer.data(), toRead, &bytesRead, nullptr) == FALSE)
        {
            // The daemon closing its end of the pipe is how a Connection: close response ends.
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                return 0;
            }

            ThrowLastError("ReadSync");
        }

        return bytesRead;
    }

    std::unique_ptr<ByteStream> DockerTransport::SendRequest(const HttpRequest& request) const
    {
        FileHandle pipeHandle = GetDockerPipeHandle();

        const std::string requestString = request.ToString();
        std::string_view remaining = requestString;
        while (!remaining.empty())
        {
            DWORD bytesWritten = 0;
            if (WriteFile(pipeHandle.get(), remaining.data(), static_cast<DWORD>(remaining.size()), &bytesWritten,
                          nullptr) == FALSE)
            {
                ThrowLastError("SendRequest");
            }

            remaining.remove_prefix(bytesWritten);
        }

        return std::make_unique<DockerConnection>(std::move(pipeHandle));
    }
} // namespace Flense::Core
