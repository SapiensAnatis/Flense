#pragma once

#include "InverseBoolToVisibilityConverter.g.h"

namespace winrt::Flense::implementation
{
    struct InverseBoolToVisibilityConverter : InverseBoolToVisibilityConverterT<InverseBoolToVisibilityConverter>
    {
        InverseBoolToVisibilityConverter() = default;

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
    struct InverseBoolToVisibilityConverter
        : InverseBoolToVisibilityConverterT<InverseBoolToVisibilityConverter, implementation::InverseBoolToVisibilityConverter>
    {
    };
} // namespace winrt::Flense::factory_implementation
