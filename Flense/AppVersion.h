#pragma once

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/base.h>

import std;

namespace winrt::Flense::implementation
{
    inline hstring GetAppVersionString()
    {
        static const hstring AppVersion = [] {
            auto package = winrt::Windows::ApplicationModel::Package::Current();
            auto version = package.Id().Version();

            return winrt::hstring(
                std::format(L"{}.{}.{}.{}", version.Major, version.Minor, version.Build, version.Revision));
        }();

        return AppVersion;
    }
} // namespace winrt::Flense::implementation