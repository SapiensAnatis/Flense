#pragma once

#include "BindingHelpers.g.h"

namespace winrt::Flense::implementation
{
    struct BindingHelpers
    {
        static bool Not(bool value);
        static bool AndNot(bool value, bool negated);
        static winrt::Microsoft::UI::Xaml::Visibility VisibleIfNotEmpty(const winrt::hstring& value);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct BindingHelpers : BindingHelpersT<BindingHelpers, implementation::BindingHelpers>
    {
    };
} // namespace winrt::Flense::factory_implementation
