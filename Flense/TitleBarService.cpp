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

    void TitleBarService::Title(const winrt::hstring& value)
    {
        if (m_title != value)
        {
            m_title = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"Title"});
        }
    }

    void TitleBarService::Reset()
    {
        Title(winrt::hstring{DefaultTitle});
    }

    event_token TitleBarService::PropertyChanged(const PropertyChangedEventHandler& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void TitleBarService::PropertyChanged(const event_token& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
} // namespace winrt::Flense::implementation
