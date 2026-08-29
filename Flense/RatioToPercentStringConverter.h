#pragma once

#include "RatioToPercentStringConverter.g.h"

namespace winrt::Flense::implementation
{
    struct RatioToPercentStringConverter : RatioToPercentStringConverterT<RatioToPercentStringConverter>
    {
        RatioToPercentStringConverter() = default;

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
    struct RatioToPercentStringConverter
        : RatioToPercentStringConverterT<RatioToPercentStringConverter, implementation::RatioToPercentStringConverter>
    {
    };
} // namespace winrt::Flense::factory_implementation
