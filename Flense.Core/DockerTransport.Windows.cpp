module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

module Flense.Core:DockerTransport.Windows;

import :DockerTransport;
import :Internal.FileHandle;

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

    std::unique_ptr<DockerConnection> DockerTransport::SendRequest(const std::string_view request) const
    {
        FileHandle pipeHandle = GetDockerPipeHandle();

        std::string_view remaining = request;
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
