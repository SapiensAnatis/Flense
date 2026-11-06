export module Flense.Core.DockerClient:DockerClient;

import :DockerTransport;
import Flense.Core.Http;

import std;

export namespace Flense::Core
{
    struct DockerEngineImage
    {
        std::string id;
        std::string repoTag;
        std::chrono::system_clock::time_point created;
        std::uint64_t size;
    };

    class DockerClient
    {
      public:
        /// <exception cref="OperationCancelledError">stopToken was triggered before the list was read.</exception>
        /// <exception cref="HttpError">An error occurred talking to the Docker engine.</exception>
        /// <exception cref="std::runtime_error">An error occurred talking to the Docker engine.</exception>
        [[nodiscard]] std::vector<DockerEngineImage> ListImages(std::stop_token stopToken = {}) const;

        /// <exception cref="OperationCancelledError">stopToken was triggered before the exort was finished.</exception>
        /// <exception cref="HttpError">An error occurred talking to the Docker engine.</exception>
        /// <exception cref="std::runtime_error">An error occurred talking to the Docker engine.</exception>
        [[nodiscard]] HttpResponse ExportImage(std::string_view tag, std::stop_token stopToken = {}) const;

      private:
        DockerTransport m_transport;
    };
} // namespace Flense::Core