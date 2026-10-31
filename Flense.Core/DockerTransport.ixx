export module Flense.Core:DockerTransport;

import :Internal.FileHandle;

import std;

export namespace Flense::Core
{
    class DockerConnection
    {
      public:
        DockerConnection(FileHandle handle) : m_handle(std::move(handle))
        {
        }

        DockerConnection(const DockerConnection&) = delete;
        DockerConnection(DockerConnection&&) = delete;

        ~DockerConnection() = default;

        DockerConnection& operator=(const DockerConnection&) = delete;
        DockerConnection& operator=(DockerConnection&&) = delete;

        std::size_t ReadSync(std::span<std::byte> buffer);

      private:
        FileHandle m_handle;
    };

    class DockerTransport
    {
      public:
        DockerTransport() = default;
        DockerTransport(const DockerTransport&) = delete;
        DockerTransport(DockerTransport&&) = delete;

        ~DockerTransport() = default;

        DockerTransport& operator=(const DockerTransport&) = delete;
        DockerTransport& operator=(DockerTransport&&) = delete;

        [[nodiscard]] std::unique_ptr<DockerConnection> SendRequest(std::string_view request) const;
    };
} // namespace Flense::Core
