export module Flense.Core:DockerClient;

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
        DockerClient() : DockerClient(DockerTransport::CreateDefault())
        {
        }

        [[nodiscard]] std::vector<DockerEngineImage> ListImages() const;

      private:
        explicit DockerClient(std::unique_ptr<DockerTransport> transport) : m_transport(std::move(transport))
        {
        }

        std::unique_ptr<DockerTransport> m_transport;
    };
} // namespace Flense::Core