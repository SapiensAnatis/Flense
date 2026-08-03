#pragma once

#include "ImageLayer.g.h"

namespace winrt::Flense::implementation
{
    struct ImageLayer : ImageLayerT<ImageLayer>
    {
        ImageLayer() = default;

        winrt::hstring Command();
    };
} // namespace winrt::Flense::implementation