export module Flense.Core:DockerTransport;

import std;

export namespace Flense::Core
{
    class DockerConnection
    {
      public:
        DockerConnection() = default;
        DockerConnection(const DockerConnection&) = delete;
        DockerConnection(DockerConnection&&) = delete;

        virtual ~DockerConnection() = default;

        DockerConnection& operator=(const DockerConnection&) = delete;
        DockerConnection& operator=(DockerConnection&&) = delete;

        virtual std::size_t ReadSync(std::span<std::byte> buffer) = 0;
    };

    class DockerTransport
    {
      public:
        DockerTransport() = default;
        DockerTransport(const DockerTransport&) = delete;
        DockerTransport(DockerTransport&&) = delete;

        virtual ~DockerTransport() = default;

        DockerTransport& operator=(const DockerTransport&) = delete;
        DockerTransport& operator=(DockerTransport&&) = delete;

        [[nodiscard]] static std::unique_ptr<DockerTransport> CreateDefault();

        virtual std::unique_ptr<DockerConnection> SendRequest(std::string_view request) = 0;
    };
} // namespace Flense::Core
