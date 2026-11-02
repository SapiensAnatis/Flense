#pragma once

#include "BindingHelpers.g.h"

namespace winrt::Flense::implementation
{
    struct BindingHelpers
    {
        static bool Not(bool value);
        static bool AndNot(bool value, bool negated);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct BindingHelpers : BindingHelpersT<BindingHelpers, implementation::BindingHelpers>
    {
    };
} // namespace winrt::Flense::factory_implementation
