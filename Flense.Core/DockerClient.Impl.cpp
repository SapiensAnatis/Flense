module Flense.Core:DockerClient.Impl;

import :DockerClient;

import nlohmann.json;

import std;

namespace Flense::Core
{
    namespace
    {
        struct DockerEngineApiImage
        {
            std::string id;
            std::vector<std::string> repoTags;
            std::uint64_t size = 0;
        };

        class ImageListHandler
        {
          public:
            [[nodiscard]] const std::vector<DockerEngineApiImage>& Images()
            {
                return m_images;
            }

            [[nodiscard]] const std::string& Error() const
            {
                return m_error;
            }

            // nlohmann's SAX parser calls these by name, so they have to use its snake_case spelling.
            // NOLINTBEGIN(readability-identifier-naming)
            bool start_array(std::size_t /* elements */)
            {
                ++m_depth;
                return true;
            }

            bool end_array()
            {
                --m_depth;
                return true;
            }

            bool start_object(std::size_t /* elements */)
            {
                ++m_depth;
                if (m_depth == ImageDepth)
                {
                    m_images.emplace_back();
                }
                return m_depth != 1;
            }

            bool end_object()
            {
                --m_depth;
                return true;
            }

            bool key(std::string& name)
            {
                if (m_depth == ImageDepth)
                {
                    m_field = ToField(name);
                }
                return true;
            }

            bool string(std::string& value)
            {
                if (m_depth == ImageDepth && m_field == Field::Id)
                {
                    m_images.back().id = std::move(value);
                }
                else if (m_depth == ImageDepth + 1 && m_field == Field::RepoTags)
                {
                    m_images.back().repoTags.push_back(std::move(value));
                }
                return true;
            }

            bool number_unsigned(nlohmann::json::number_unsigned_t value)
            {
                if (m_depth == ImageDepth)
                {
                    StoreNumber(value);
                }
                return true;
            }

            bool number_integer(nlohmann::json::number_integer_t value)
            {
                if (m_depth == ImageDepth && value >= 0)
                {
                    StoreNumber(static_cast<std::uint64_t>(value));
                }
                return true;
            }

            static bool number_float(nlohmann::json::number_float_t /* value */, const std::string& /* text */)
            {
                return true;
            }

            static bool boolean(bool /* value */)
            {
                return true;
            }

            static bool null()
            {
                return true;
            }

            static bool binary(nlohmann::json::binary_t& /* value */)
            {
                return true;
            }

            bool parse_error(std::size_t position, const std::string& /* token */,
                             const nlohmann::json::exception& error)
            {
                m_error = std::format("Invalid image list JSON at byte {}: {}", position, error.what());
                return false;
            }
            // NOLINTEND(readability-identifier-naming)

          private:
            enum class Field : std::uint8_t
            {
                Other,
                Id,
                RepoTags,
                Size,
                Created,
            };

            static constexpr int ImageDepth = 2;

            static Field ToField(std::string_view name)
            {
                if (name == "Id")
                {
                    return Field::Id;
                }
                if (name == "RepoTags")
                {
                    return Field::RepoTags;
                }
                if (name == "Size")
                {
                    return Field::Size;
                }
                return Field::Other;
            }

            void StoreNumber(std::uint64_t value)
            {
                if (m_field == Field::Size)
                {
                    m_images.back().size = value;
                }
            }

            std::vector<DockerEngineApiImage> m_images;
            std::string m_error;
            Field m_field = Field::Other;
            int m_depth = 0;
        };
    } // namespace

    std::vector<DockerEngineImage> DockerClient::ListImages() const
    {
        // TODO: Pin an API version (e.g. /v1.41/images/json) so the response format doesn't change when the
        // daemon is upgraded.
        const std::string request = "GET /images/json HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
        const std::string response = m_transport.SendRequest(request);

        // TODO: Check the status line. Error responses carry a {"message": ...} body that we should surface instead
        // of failing to parse it as an image list.
        // TODO: Handle Transfer-Encoding: chunked. Docker doesn't send a Content-Length for JSON responses, so
        // larger image lists will arrive chunked and the chunk size lines will break the parse.
        auto responseBodyStart = response.find("\r\n\r\n");

        if (responseBodyStart == std::string::npos)
        {
            throw std::runtime_error("Invalid HTTP response: missing header-body separator");
        }

        const std::string_view responseBody =
            std::string_view(response).substr(responseBodyStart + std::string_view("\r\n\r\n").size());

        ImageListHandler handler;
        if (!nlohmann::json::sax_parse(responseBody, &handler))
        {
            throw std::runtime_error("Failed to parse image list JSON");
        }

        std::vector<DockerEngineImage> images;
        images.reserve(handler.Images().size());

        for (const auto& apiImage : handler.Images())
        {
            for (const auto& repoTag : apiImage.repoTags) // Only include images with at least one repo tag
            {
                images.emplace_back(apiImage.id, repoTag, apiImage.size);
            }
        }

        return images;
    }
} // namespace Flense::Core