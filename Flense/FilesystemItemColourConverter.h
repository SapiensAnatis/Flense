#pragma once

#include "FilesystemItemColourConverter.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemItemColourConverter : FilesystemItemColourConverterT<FilesystemItemColourConverter>
    {
        FilesystemItemColourConverter() = default;

        Microsoft::UI::Xaml::Style AddedStyle() const
        {
            return m_addedStyle;
        }

        void AddedStyle(const Microsoft::UI::Xaml::Style& value)
        {
            m_addedStyle = value;
        }

        Microsoft::UI::Xaml::Style RemovedStyle() const
        {
            return m_removedStyle;
        }

        void RemovedStyle(const Microsoft::UI::Xaml::Style& value)
        {
            m_removedStyle = value;
        }

        Microsoft::UI::Xaml::Style ModifiedStyle() const
        {
            return m_modifiedStyle;
        }

        void ModifiedStyle(const Microsoft::UI::Xaml::Style& value)
        {
            m_modifiedStyle = value;
        }

        Windows::Foundation::IInspectable Convert(const Windows::Foundation::IInspectable& value,
                                                  const Windows::UI::Xaml::Interop::TypeName& targetType,
                                                  const Windows::Foundation::IInspectable& parameter,
                                                  const hstring& language);

        Windows::Foundation::IInspectable ConvertBack(const Windows::Foundation::IInspectable& value,
                                                      const Windows::UI::Xaml::Interop::TypeName& targetType,
                                                      const Windows::Foundation::IInspectable& parameter,
                                                      const hstring& language);

      private:
        Microsoft::UI::Xaml::Style m_addedStyle{nullptr};
        Microsoft::UI::Xaml::Style m_removedStyle{nullptr};
        Microsoft::UI::Xaml::Style m_modifiedStyle{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemItemColourConverter
        : FilesystemItemColourConverterT<FilesystemItemColourConverter, implementation::FilesystemItemColourConverter>
    {
    };
} // namespace winrt::Flense::factory_implementation
