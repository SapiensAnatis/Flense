module Flense.Core.DockerClient:DockerClient.Impl;

import Flense.Core.Http;
import Flense.Core.Shared;

import :DockerClient;

import nlohmann.json;

import std;

namespace Flense::Core
{
    namespace
    {
        constexpr std::string_view ApiVersion = "v1.41";

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

    std::vector<DockerEngineImage> DockerClient::ListImages(std::stop_token stopToken) const
    {
        HttpRequest request(HttpMethod::Get, std::format("/{}/images/json", ApiVersion));
        request.AddHeader("Host", "localhost");
        request.AddHeader("Connection", "close");
        request.AddHeader("Accept", "application/json");

        HttpResponse response(m_transport.SendRequest(request, stopToken));

        // TODO: Error responses carry a {"message": ...} body that we should surface.
        if (response.StatusCode() != 200)
        {
            throw std::runtime_error(std::format("Docker returned HTTP status {}", response.StatusCode()));
        }

        ByteStreamRange responseBody(response);

        ImageListHandler handler;
        if (!nlohmann::json::sax_parse(responseBody.begin(), responseBody.end(), &handler))
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