export module Flense.Core.DockerClient:DockerClient;

import :DockerTransport;

import std;

export namespace Flense::Core
{
    struct DockerEngineImage
    {
        std::string id;
        std::string repoTag;
        std::uint64_t size;
    };

    class DockerClient
    {
      public:
        /// <exception cref="OperationCancelledError">stopToken was triggered before the list was read.</exception>
        [[nodiscard]] std::vector<DockerEngineImage> ListImages(std::stop_token stopToken = {}) const;

      private:
        DockerTransport m_transport;
    };
} // namespace Flense::Core