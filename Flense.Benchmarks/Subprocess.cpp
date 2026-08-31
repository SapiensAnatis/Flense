#include "pch.h"

#include "Subprocess.h"

#include <array>
#include <format>
#include <memory>
#include <stdexcept>

namespace Flense::Benchmarks
{
    namespace
    {
        struct HandleCloser
        {
            void operator()(const HANDLE handle) const noexcept
            {
                if (handle != nullptr && handle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(handle);
                }
            }
        };

        using UniqueHandle = std::unique_ptr<void, HandleCloser>;

        /// <summary>
        /// Quotes a single argument for CreateProcessW's command line, per the escaping rules the
        /// Windows CRT uses to split a command line back into argv.
        /// </summary>
        /// <remarks>
        /// A literal quote must be preceded by an odd number of backslashes to survive round-tripping,
        /// and any backslashes immediately before the *closing* quote must be doubled so they aren't
        /// mistaken for an escape of that closing quote - e.g. an argument that is itself a path ending
        /// in a backslash.
        /// </remarks>
        /// <param name="arg">The argument to quote.</param>
        /// <returns>The quoted argument.</returns>
        std::wstring QuoteArgument(const std::wstring& arg)
        {
            std::wstring result = L"\"";

            size_t backslashCount = 0;
            for (const wchar_t c : arg)
            {
                if (c == L'\\')
                {
                    backslashCount += 1;
                    continue;
                }

                if (c == L'"')
                {
                    result.append(backslashCount * 2 + 1, L'\\');
                }
                else
                {
                    result.append(backslashCount, L'\\');
                }

                backslashCount = 0;
                result.push_back(c);
            }

            result.append(backslashCount * 2, L'\\');
            result.push_back(L'"');

            return result;
        }

        /// <summary>
        /// Builds a command line from a list of arguments, each individually quoted.
        /// </summary>
        std::wstring BuildCommandLine(const std::wstring& exePath, const std::vector<std::wstring>& args)
        {
            std::wstring commandLine = QuoteArgument(exePath);

            for (const std::wstring& arg : args)
            {
                commandLine += L' ';
                commandLine += QuoteArgument(arg);
            }

            return commandLine;
        }
    } // namespace

    std::string RunSelfCapturingStdout(const std::vector<std::wstring>& args)
    {
        std::array<wchar_t, MAX_PATH> modulePath{};
        if (GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size())) == 0)
        {
            throw std::runtime_error("Could not determine the running executable's path.");
        }

        SECURITY_ATTRIBUTES pipeAttributes{
            .nLength = sizeof(SECURITY_ATTRIBUTES),
            .lpSecurityDescriptor = nullptr,
            .bInheritHandle = TRUE,
        };

        HANDLE readHandleRaw = nullptr;
        HANDLE writeHandleRaw = nullptr;
        if (!CreatePipe(&readHandleRaw, &writeHandleRaw, &pipeAttributes, 0))
        {
            throw std::runtime_error("Could not create a pipe for the child process's output.");
        }

        const UniqueHandle readHandle{readHandleRaw};
        UniqueHandle writeHandle{writeHandleRaw};

        // The parent's end of the pipe must not be inherited by the child, or the pipe never sees EOF.
        if (!SetHandleInformation(readHandle.get(), HANDLE_FLAG_INHERIT, 0))
        {
            throw std::runtime_error("Could not configure the output pipe.");
        }

        STARTUPINFOW startupInfo{
            .cb = sizeof(STARTUPINFOW),
            .dwFlags = STARTF_USESTDHANDLES,
            .hStdInput = GetStdHandle(STD_INPUT_HANDLE),
            .hStdOutput = writeHandle.get(),
            .hStdError = GetStdHandle(STD_ERROR_HANDLE),
        };

        PROCESS_INFORMATION processInfo{};

        std::wstring commandLine = BuildCommandLine(modulePath.data(), args);

        const BOOL created = CreateProcessW(modulePath.data(), commandLine.data(), nullptr, nullptr, TRUE, 0,
                                            nullptr, nullptr, &startupInfo, &processInfo);

        // The child inherited its own copy of the write end; the parent's copy must be closed now, or
        // the ReadFile loop below blocks forever waiting for a write end that will never close.
        writeHandle.reset();

        if (!created)
        {
            throw std::runtime_error(std::format("Could not start the child benchmark process (error {}).",
                                                 GetLastError()));
        }

        const UniqueHandle processHandle{processInfo.hProcess};
        const UniqueHandle threadHandle{processInfo.hThread};

        std::string output;
        std::array<char, 4096> buffer{};
        DWORD bytesRead = 0;
        while (ReadFile(readHandle.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) &&
               bytesRead > 0)
        {
            output.append(buffer.data(), bytesRead);
        }

        WaitForSingleObject(processHandle.get(), INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(processHandle.get(), &exitCode);

        if (exitCode != 0)
        {
            throw std::runtime_error(std::format("Child benchmark process exited with code {}.", exitCode));
        }

        return output;
    }
} // namespace Flense::Benchmarks
