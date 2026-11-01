module;

#include <cassert>

module Flense.Core.Http:HttpRequest.Impl;

import :HttpRequest;

import std;

namespace Flense::Core
{
    namespace
    {
        std::string_view HttpMethodToString(HttpMethod method)
        {
            switch (method)
            {
            case HttpMethod::Get:
                return "GET";
            }

            assert(false && "Unknown HTTP method");
            return "UNKNOWN";
        }
    } // namespace

    [[nodiscard]] std::string HttpRequest::ToString() const
    {
        std::string request = std::format("{} {} HTTP/1.1\r\n", HttpMethodToString(m_method), m_path);
        for (const auto& [name, value] : m_headers)
        {
            request += std::format("{}: {}\r\n", name, value);
        }
        request += "\r\n";
        return request;
    }
} // namespace Flense::Core
