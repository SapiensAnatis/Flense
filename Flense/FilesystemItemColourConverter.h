#pragma once

#include "FilesystemItemColourConverter.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemItemColourConverter : FilesystemItemColourConverterT<FilesystemItemColourConverter>
    {
        FilesystemItemColourConverter() = default;

        void InitializeComponent();

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
    struct FilesystemItemColourConverter
        : FilesystemItemColourConverterT<FilesystemItemColourConverter, implementation::FilesystemItemColourConverter>
    {
    };
} // namespace winrt::Flense::factory_implementation
