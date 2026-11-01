export module Flense.Core.Http:HttpRequest;

import std;

export namespace Flense::Core
{
    enum class HttpMethod : std::uint8_t
    {
        Get,
    };

    class HttpRequest
    {
      public:
        HttpRequest(HttpMethod method, std::string_view path) : m_method(method), m_path(path)
        {
        }

        void AddHeader(std::string_view name, std::string_view value)
        {
            m_headers.emplace_back(name, value);
        }

        [[nodiscard]] std::string ToString() const;

      private:
        HttpMethod m_method;
        std::string m_path;
        std::vector<std::pair<std::string, std::string>> m_headers;
    };
} // namespace Flense::Core
