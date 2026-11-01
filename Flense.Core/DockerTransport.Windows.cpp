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
        [[noreturn]] void ThrowError(const DWORD error, const char* operation)
        {
            throw std::system_error(static_cast<int>(error), std::system_category(), operation);
        }

        [[noreturn]] void ThrowLastError(const char* operation)
        {
            ThrowError(GetLastError(), operation);
        }

        FileHandle CreateManualResetEvent()
        {
            FileHandle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!event)
            {
                ThrowLastError("CreateEventW");
            }

            return event;
        }

        struct OverlappedResult
        {
            DWORD bytesTransferred = 0;
            DWORD error = ERROR_SUCCESS;
        };

        // A synchronous ReadFile on the pipe would block until the daemon writes, which could be indefinitely, so
        // I/O is overlapped and abandoned with CancelIoEx once the stop token is triggered.
        template <std::invocable<OVERLAPPED&> StartIo>
        OverlappedResult RunOverlapped(HANDLE handle, const std::stop_token& stopToken, StartIo startIo)
        {
            if (stopToken.stop_requested())
            {
                throw OperationCancelledError{};
            }

            const FileHandle ioEvent = CreateManualResetEvent();
            const FileHandle stopEvent = CreateManualResetEvent();

            OVERLAPPED overlapped{};
            overlapped.hEvent = ioEvent.get();

            if (startIo(overlapped) == FALSE)
            {
                if (const DWORD error = GetLastError(); error != ERROR_IO_PENDING)
                {
                    return {.error = error};
                }
            }

            {
                const std::stop_callback onStop(stopToken, [&stopEvent] { SetEvent(stopEvent.get()); });

                const std::array events{ioEvent.get(), stopEvent.get()};
                if (WaitForMultipleObjects(static_cast<DWORD>(events.size()), events.data(), FALSE, INFINITE) !=
                    WAIT_OBJECT_0)
                {
                    CancelIoEx(handle, &overlapped);
                }
            }

            OverlappedResult result;
            if (GetOverlappedResult(handle, &overlapped, &result.bytesTransferred, TRUE) == FALSE)
            {
                result.error = GetLastError();
                if (result.error == ERROR_OPERATION_ABORTED)
                {
                    throw OperationCancelledError{};
                }
            }

            return result;
        }

        FileHandle GetDockerPipeHandle()
        {
            // TODO: Respect DOCKER_HOST so that users on a non-default context reach the right daemon.
            // TODO: Retry with WaitNamedPipeW when this fails with ERROR_PIPE_BUSY.
            const std::wstring pipeName = LR"(\\.\pipe\docker_engine)";
            HANDLE pipeHandle = CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                            FILE_FLAG_OVERLAPPED, nullptr);
            if (pipeHandle == INVALID_HANDLE_VALUE)
            {
                ThrowLastError("Failed to open Docker named pipe");
            }

            return FileHandle(pipeHandle);
        }

    } // namespace

    std::size_t DockerConnection::ReadSync(const std::span<std::byte> buffer)
    {
        const DWORD toRead =
            static_cast<DWORD>(std::min<std::size_t>(buffer.size(), std::numeric_limits<DWORD>::max()));

        const auto [bytesRead, error] = RunOverlapped(m_handle.get(), m_stopToken, [&](OVERLAPPED& overlapped) {
            return ReadFile(m_handle.get(), buffer.data(), toRead, nullptr, &overlapped);
        });

        // The daemon closing its end of the pipe is how a Connection: close response ends.
        if (error != ERROR_SUCCESS && error != ERROR_BROKEN_PIPE)
        {
            ThrowError(error, "ReadSync");
        }

        return bytesRead;
    }

    std::unique_ptr<ByteStream> DockerTransport::SendRequest(const HttpRequest& request,
                                                             std::stop_token stopToken) const
    {
        FileHandle pipeHandle = GetDockerPipeHandle();

        const std::string requestString = request.ToString();
        std::string_view remaining = requestString;
        while (!remaining.empty())
        {
            const auto [bytesWritten, error] = RunOverlapped(pipeHandle.get(), stopToken, [&](OVERLAPPED& overlapped) {
                return WriteFile(pipeHandle.get(), remaining.data(), static_cast<DWORD>(remaining.size()), nullptr,
                                 &overlapped);
            });

            if (error != ERROR_SUCCESS)
            {
                ThrowError(error, "SendRequest");
            }

            remaining.remove_prefix(bytesWritten);
        }

        return std::make_unique<DockerConnection>(std::move(pipeHandle), stopToken);
    }
} // namespace Flense::Core
