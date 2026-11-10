#pragma once

#include "License.g.h"

namespace winrt::Flense::implementation
{
    struct License : LicenseT<License>
    {
        License(hstring name, hstring notice) : Name(name), Notice(notice)
        {
        }

        wil::single_threaded_property<hstring> Name;
        wil::single_threaded_property<hstring> Notice;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct License : LicenseT<License, implementation::License>
    {
    };
} // namespace winrt::Flense::factory_implementation
