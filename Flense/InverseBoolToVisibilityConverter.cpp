#include "pch.h"

#include "InverseBoolToVisibilityConverter.h"
#if __has_include("InverseBoolToVisibilityConverter.g.cpp")
#include "InverseBoolToVisibilityConverter.g.cpp"
#endif

using namespace winrt::Windows::UI::Xaml::Interop;
using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::Flense::implementation
{
    winrt::Windows::Foundation::IInspectable InverseBoolToVisibilityConverter::Convert(
        const winrt::Windows::Foundation::IInspectable& value, const TypeName& /* targetType */,
        const winrt::Windows::Foundation::IInspectable& /* parameter */, const hstring& /* language */)
    {
        const bool boolValue = winrt::unbox_value_or<bool>(value, false);
        return winrt::box_value(boolValue ? Visibility::Collapsed : Visibility::Visible);
    }

    winrt::Windows::Foundation::IInspectable InverseBoolToVisibilityConverter::ConvertBack(
        const winrt::Windows::Foundation::IInspectable& value, const TypeName& /* targetType */,
        const winrt::Windows::Foundation::IInspectable& /* parameter */, const hstring& /* language */)
    {
        const auto visibility = winrt::unbox_value_or<Visibility>(value, Visibility::Visible);
        return winrt::box_value(visibility == Visibility::Collapsed);
    }
} // namespace winrt::Flense::implementation
