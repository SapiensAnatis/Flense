#pragma once

#include "RatioToGridLengthConverter.g.h"

namespace winrt::Flense::implementation
{
    struct RatioToGridLengthConverter : RatioToGridLengthConverterT<RatioToGridLengthConverter>
    {
        RatioToGridLengthConverter() = default;

        Windows::Foundation::IInspectable Convert(const Windows::Foundation::IInspectable& value,
                                                  const Windows::UI::Xaml::Interop::TypeName& targetType,
                                                  const Windows::Foundation::IInspectable& parameter,
                                                  const hstring& language);

        Windows::Foundation::IInspectable ConvertBack(const Windows::Foundation::IInspectable& value,
                                                      const Windows::UI::Xaml::Interop::TypeName& targetType,
                                                      const Windows::Foundation::IInspectable& parameter,
                                                      const hstring& language);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct RatioToGridLengthConverter
        : RatioToGridLengthConverterT<RatioToGridLengthConverter, implementation::RatioToGridLengthConverter>
    {
    };
} // namespace winrt::Flense::factory_implementation
