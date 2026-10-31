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
        [[nodiscard]] std::vector<DockerEngineImage> ListImages() const;

      private:
        DockerTransport m_transport;
    };
} // namespace Flense::Core