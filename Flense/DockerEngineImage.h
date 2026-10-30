#pragma once

#include "DockerEngineImage.g.h"

import Flense.Core;

namespace winrt::Flense::implementation
{
    struct DockerEngineImage : DockerEngineImageT<DockerEngineImage>
    {
        DockerEngineImage(const ::Flense::Core::DockerEngineImage& image);

        wil::single_threaded_property<winrt::hstring> Id;
        wil::single_threaded_property<winrt::hstring> RepoTag;
        wil::single_threaded_property<uint64_t> Size;
    };
} // namespace winrt::Flense::implementation
