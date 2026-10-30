export module Flense.Core:DockerTransport;

import std;

export namespace Flense::Core
{
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

        virtual std::string SendRequest(std::string_view request) = 0;
    };
} // namespace Flense::Core
