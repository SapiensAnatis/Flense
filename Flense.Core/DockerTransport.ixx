export module Flense.Core.DockerClient:DockerTransport;

import Flense.Core.Http;
import Flense.Core.Shared;

import std;

export namespace Flense::Core
{
    class DockerConnection : public ByteStream
    {
      public:
        explicit DockerConnection(FileHandle handle) : m_handle(std::move(handle))
        {
        }

        DockerConnection(const DockerConnection&) = delete;
        DockerConnection(DockerConnection&&) = delete;

        ~DockerConnection() override = default;

        DockerConnection& operator=(const DockerConnection&) = delete;
        DockerConnection& operator=(DockerConnection&&) = delete;

        std::size_t ReadSync(std::span<std::byte> buffer) override;

      private:
        FileHandle m_handle;
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

        [[nodiscard]] virtual std::unique_ptr<ByteStream> SendRequest(const HttpRequest& request) const;
    };
} // namespace Flense::Core
