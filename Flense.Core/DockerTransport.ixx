export module Flense.Core.DockerClient:DockerTransport;

import Flense.Core.Http;
import Flense.Core.Shared;

import std;

export namespace Flense::Core
{
    class DockerConnection : public ByteStream
    {
      public:
        DockerConnection(FileHandle handle, std::stop_token stopToken)
            : m_handle(std::move(handle)), m_stopToken(std::move(stopToken))
        {
        }

        DockerConnection(const DockerConnection&) = delete;
        DockerConnection(DockerConnection&&) = delete;

        ~DockerConnection() override = default;

        DockerConnection& operator=(const DockerConnection&) = delete;
        DockerConnection& operator=(DockerConnection&&) = delete;

        /// <exception cref="OperationCancelledError">The stop token was triggered before or during the read.</exception>
        std::size_t ReadSync(std::span<std::byte> buffer) override;

      private:
        FileHandle m_handle;
        std::stop_token m_stopToken;
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

        /// <summary>
        /// Sends a request to the daemon and returns the connection to read its response from. Reads from the
        /// connection stop early once stopToken is triggered.
        /// </summary>
        /// <exception cref="OperationCancelledError">stopToken was triggered before the request was sent.</exception>
        [[nodiscard]] virtual std::unique_ptr<ByteStream> SendRequest(const HttpRequest& request,
                                                                      std::stop_token stopToken) const;
    };
} // namespace Flense::Core
