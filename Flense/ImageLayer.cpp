#include "pch.h"
#include "ImageLayer.h"
#if __has_include("ImageLayer.g.cpp")
#include "ImageLayer.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    int32_t ImageLayer::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void ImageLayer::MyProperty(int32_t /*value*/)
    {
        throw hresult_not_implemented();
    }
}
