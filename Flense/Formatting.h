#pragma once

#include "Formatting.g.h"

namespace winrt::Flense::implementation
{
    struct Formatting
    {
        static hstring FormatDate(const Windows::Foundation::DateTime& value);
        static hstring FormatRelativeTime(const Windows::Foundation::DateTime& value);
        static hstring FormatLinkTarget(const hstring& value);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct Formatting : FormattingT<Formatting, implementation::Formatting>
    {
    };
} // namespace winrt::Flense::factory_implementation
