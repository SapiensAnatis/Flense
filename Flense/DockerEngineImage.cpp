#include "pch.h"

#include "DockerEngineImage.h"
#if __has_include("DockerEngineImage.g.cpp")
#include "DockerEngineImage.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    DockerEngineImage::DockerEngineImage(const ::Flense::Core::DockerEngineImage& image)
        : Id(winrt::to_hstring(image.id)), RepoTag(winrt::to_hstring(image.repoTag)), Size(image.size)
    {
    }
} // namespace winrt::Flense::implementation
