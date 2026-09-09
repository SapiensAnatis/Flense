#include "pch.h"

#include "TitleBarService.h"
#if __has_include("TitleBarService.g.cpp")
#include "TitleBarService.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml::Data;

namespace winrt::Flense::implementation
{
    winrt::Flense::TitleBarService TitleBarService::Instance()
    {
        static winrt::Flense::TitleBarService instance = winrt::make<TitleBarService>();
        return instance;
    }

    void TitleBarService::Reset()
    {
        Title(winrt::hstring{DefaultTitle});
    }
} // namespace winrt::Flense::implementation
