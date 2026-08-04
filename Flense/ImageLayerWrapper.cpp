#include "pch.h"

#include "ImageLayerWrapper.h"
#if __has_include("ImageLayerWrapper.g.cpp")
#include "ImageLayerWrapper.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    ImageLayerWrapper::ImageLayerWrapper(::Flense::Core::ImageLayer layer) : m_layer(std::move(layer))
    {
    }

    winrt::hstring ImageLayerWrapper::Command()
    {
        // TODO: Cache strings
        return winrt::to_hstring(m_layer.Command());
    }
} // namespace winrt::Flense::implementation
