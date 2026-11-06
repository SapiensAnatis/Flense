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
        /// <summary>
        /// The Docker Engine API version.
        /// 1.47 gives us support for manifests from GET /images/json, which allows us to estimate the size of an image
        /// tar file for use in the progress bar.
        /// </summary>
        constexpr std::string_view ApiVersion = "v1.47";

        struct DockerEngineApiImage
        {
            std::string id;
            std::vector<std::string> repoTags;
            std::chrono::system_clock::time_point created;
            std::uint64_t size = 0;
            std::uint64_t estimatedTarSize = 0;
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
                PushPendingKey();
                return true;
            }

            bool end_array()
            {
                m_path.pop_back();
                return true;
            }

            bool start_object(std::size_t /* elements */)
            {
                if (m_path.empty())
                {
                    // We expect the top-level element to be an array
                    return false;
                }

                static constexpr auto ImageObjectPath = std::array{PathPart::Root, PathPart::ArrayElement};

                PushPendingKey();
                if (std::ranges::equal(m_path, ImageObjectPath))
                {
                    m_images.emplace_back();
                }
                return true;
            }

            bool end_object()
            {
                m_path.pop_back();
                return true;
            }

            bool key(std::string& name)
            {
                m_pendingKey = ToPathPart(name);
                return true;
            }

            bool string(std::string& value)
            {
                static constexpr auto ImageIdPath = std::array{PathPart::Root, PathPart::ArrayElement, PathPart::Id};

                static constexpr auto RepoTagsPath =
                    std::array{PathPart::Root, PathPart::ArrayElement, PathPart::RepoTags, PathPart::ArrayElement};

                PushPendingKey();

                if (std::ranges::equal(m_path, ImageIdPath))
                {
                    m_images.back().id = std::move(value);
                }
                else if (std::ranges::equal(m_path, RepoTagsPath))
                {
                    m_images.back().repoTags.push_back(std::move(value));
                }

                m_path.pop_back();
                return true;
            }

            bool number_unsigned(nlohmann::json::number_unsigned_t value)
            {
                PushPendingKey();

                StoreNumber(value);

                m_path.pop_back();
                return true;
            }

            bool number_integer(nlohmann::json::number_integer_t value)
            {
                PushPendingKey();

                if (value >= 0)
                {
                    StoreNumber(static_cast<std::uint64_t>(value));
                }

                m_path.pop_back();
                return true;
            }

            bool number_float(nlohmann::json::number_float_t /* value */, const std::string& /* text */)
            {
                ResetPendingKey();
                return true;
            }

            bool boolean(bool /* value */)
            {
                ResetPendingKey();
                return true;
            }

            bool null()
            {
                ResetPendingKey();
                return true;
            }

            bool binary(nlohmann::json::binary_t& /* value */)
            {
                ResetPendingKey();
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
            enum class PathPart : std::uint8_t
            {
                Unknown,
                Root,
                ArrayElement,
                Id,
                RepoTags,
                Size,
                Created,
                Manifests,
                Content,
            };

            static PathPart ToPathPart(std::string_view name)
            {
                if (name == "Id")
                {
                    return PathPart::Id;
                }
                if (name == "RepoTags")
                {
                    return PathPart::RepoTags;
                }
                if (name == "Size")
                {
                    return PathPart::Size;
                }
                if (name == "Created")
                {
                    return PathPart::Created;
                }
                if (name == "Manifests")
                {
                    return PathPart::Manifests;
                }
                if (name == "Content")
                {
                    return PathPart::Content;
                }
                return PathPart::Unknown;
            }

            void PushPendingKey()
            {
                m_path.push_back(m_pendingKey);
                ResetPendingKey();
            }

            void ResetPendingKey()
            {
                m_pendingKey =
                    PathPart::ArrayElement; // Default to array element for the next value if no key is provided
            }

            void StoreNumber(std::uint64_t value)
            {
                static constexpr auto ImageSizePath =
                    std::array{PathPart::Root, PathPart::ArrayElement, PathPart::Size};

                static constexpr auto ImageCreatedPath =
                    std::array{PathPart::Root, PathPart::ArrayElement, PathPart::Created};

                static constexpr auto ImageManifestContentSizePath =
                    std::array{PathPart::Root,         PathPart::ArrayElement, PathPart::Manifests,
                               PathPart::ArrayElement, PathPart::Size,         PathPart::Content};

                if (std::ranges::equal(m_path, ImageSizePath))
                {
                    m_images.back().size = value;
                }
                else if (std::ranges::equal(m_path, ImageCreatedPath))
                {
                    constexpr auto MaxSecs = static_cast<std::uint64_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::duration::max())
                            .count());

                    if (value < MaxSecs)
                    {
                        m_images.back().created =
                            std::chrono::system_clock::from_time_t(static_cast<std::time_t>(value));
                    }
                }
                else if (std::ranges::equal(m_path, ImageManifestContentSizePath))
                {
                    m_images.back().estimatedTarSize += value;
                }
            }

            std::vector<DockerEngineApiImage> m_images;
            std::string m_error;
            PathPart m_pendingKey = PathPart::Root;
            std::vector<PathPart> m_path;
        };

        std::string UrlEncode(const std::string_view value)
        {
            std::string escaped;
            escaped.reserve(value.size() * 3);

            for (const char c : value)
            {
                auto uc = static_cast<unsigned char>(c);
                if (std::isalnum(uc) != 0 || c == '-' || c == '_' || c == '.' || c == '~')
                {
                    escaped.push_back(c);
                }
                else
                {
                    escaped += std::format("%{:02X}", uc);
                }
            }

            return escaped;
        }
    } // namespace

    std::vector<DockerEngineImage> DockerClient::ListImages(std::stop_token stopToken) const
    {
        HttpRequest request(HttpMethod::Get, std::format("/{}/images/json?manifests=1", ApiVersion));
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

        // Flatten images with multiple repo tags into one image per tag
        for (const auto& apiImage : handler.Images())
        {
            for (const auto& repoTag : apiImage.repoTags)
            {
                if (repoTag != "<none>:<none>") // Skip dangling images
                {
                    images.emplace_back(apiImage.id, repoTag, apiImage.created, apiImage.size,
                                        apiImage.estimatedTarSize);
                }
            }
        }

        // The API already returns images in chronological order, but we'll do our own sort to be double sure in case
        // this undocumented behaviour changes
        std::ranges::sort(images, [](const DockerEngineImage& lhs, const DockerEngineImage& rhs) {
            if (lhs.created != rhs.created)
            {
                return lhs.created > rhs.created;
            }

            // We flatten the repo tags into a single list, so use the tag as a tie-breaker to get a stable order for
            // images with multiple tags which would otherwise have identical build dates.
            return lhs.repoTag < rhs.repoTag;
        });

        return images;
    }

    HttpResponse DockerClient::ExportImage(const std::string_view tag, const std::stop_token stopToken) const
    {
        HttpRequest request(HttpMethod::Get, std::format("/{}/images/{}/get", ApiVersion, UrlEncode(tag)));
        request.AddHeader("Host", "localhost");
        request.AddHeader("Connection", "close");
        request.AddHeader("Accept", "application/x-tar");

        HttpResponse response(m_transport.SendRequest(request, stopToken));

        if (response.StatusCode() != 200)
        {
            throw std::runtime_error(std::format("Docker returned HTTP status {}", response.StatusCode()));
        }

        return response;
    }
} // namespace Flense::Core