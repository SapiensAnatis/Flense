#include "pch.h"

#include "BindingHelpers.h"
#if __has_include("BindingHelpers.g.cpp")
#include "BindingHelpers.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    bool BindingHelpers::Not(bool value)
    {
        return !value;
    }

    bool BindingHelpers::AndNot(const bool value, const bool negated)
    {
        return value && !negated;
    }

    // This returns Visibility rather than bool because the XAML compiler generates C++ that doesn't compile
    // when a function binding relies on the implicit bool to Visibility conversion.
    winrt::Microsoft::UI::Xaml::Visibility BindingHelpers::VisibleIfNotEmpty(const winrt::hstring& value)
    {
        using winrt::Microsoft::UI::Xaml::Visibility;

        return value.empty() ? Visibility::Collapsed : Visibility::Visible;
    }
} // namespace winrt::Flense::implementation