#pragma once

#include <winrt/base.h>

namespace winrt::Flense::implementation
{
    bool ContainsCaseInsensitive(const winrt::hstring& haystack, const winrt::hstring& needle);
    bool ContainsCaseInsensitive(const winrt::hstring& haystack, const std::wstring_view& needle);
} // namespace winrt::Flense::implementation