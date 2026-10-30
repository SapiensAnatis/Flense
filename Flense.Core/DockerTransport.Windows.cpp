module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

module Flense.Core:DockerTransport.Windows;

import :DockerTransport;

import std;

namespace Flense::Core
{
    namespace
    {
        struct HandleCloser
        {
            void operator()(HANDLE handle) const noexcept
            {
                CloseHandle(handle);
            }
        };

        using HandlePtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

        [[noreturn]] void ThrowLastError(const char* operation)
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
        }

        HandlePtr GetDockerPipeHandle()
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

            return HandlePtr(pipeHandle);
        }

        class WindowsDockerTransport : public DockerTransport
        {
          public:
            std::string SendRequest(const std::string_view request) override
            {
                const HandlePtr pipeHandle = GetDockerPipeHandle();

                std::string_view remaining = request;
                while (!remaining.empty())
                {
                    DWORD bytesWritten = 0;
                    if (WriteFile(pipeHandle.get(), remaining.data(), static_cast<DWORD>(remaining.size()),
                                  &bytesWritten, nullptr) == FALSE)
                    {
                        ThrowLastError("SendRequest");
                    }

                    remaining.remove_prefix(bytesWritten);
                }

                // TODO: Add a timeout, since a hung daemon will block this read forever. Overlapped I/O would also
                // let callers cancel the request.
                // TODO: Cap the response size so a huge image list can't exhaust memory.
                std::string response;
                std::array<char, 4096> buffer{};
                while (true)
                {
                    DWORD bytesRead = 0;
                    if (ReadFile(pipeHandle.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead,
                                 nullptr) == FALSE)
                    {
                        if (GetLastError() == ERROR_BROKEN_PIPE)
                        {
                            break;
                        }

                        ThrowLastError("ReadFile");
                    }

                    if (bytesRead == 0)
                    {
                        break;
                    }

                    response.append(buffer.data(), bytesRead);
                }

                return response;
            }
        };
    } // namespace

    std::unique_ptr<DockerTransport> DockerTransport::CreateDefault()
    {
        return std::make_unique<WindowsDockerTransport>();
    }
} // namespace Flense::Core
