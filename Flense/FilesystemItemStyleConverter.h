#pragma once

#include "FilesystemItemStyleConverter.g.h"

namespace winrt::Flense::implementation
{
    // Maps a change kind onto a TreeViewItem style. It hands back styles rather than brushes so that
    // the colours stay in XAML: a style's ThemeResource setters are re-resolved by the framework when
    // the theme changes, whereas a brush handed out by a converter is whatever it was when the binding
    // last ran.
    struct FilesystemItemStyleConverter : FilesystemItemStyleConverterT<FilesystemItemStyleConverter>
    {
        FilesystemItemStyleConverter() = default;

        Microsoft::UI::Xaml::Style Added() const noexcept
        {
            return m_added;
        }

        void Added(const Microsoft::UI::Xaml::Style& value)
        {
            m_added = value;
        }

        Microsoft::UI::Xaml::Style Removed() const noexcept
        {
            return m_removed;
        }

        void Removed(const Microsoft::UI::Xaml::Style& value)
        {
            m_removed = value;
        }

        Microsoft::UI::Xaml::Style Modified() const noexcept
        {
            return m_modified;
        }

        void Modified(const Microsoft::UI::Xaml::Style& value)
        {
            m_modified = value;
        }

        Microsoft::UI::Xaml::Style Unchanged() const noexcept
        {
            return m_unchanged;
        }

        void Unchanged(const Microsoft::UI::Xaml::Style& value)
        {
            m_unchanged = value;
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
        Microsoft::UI::Xaml::Style m_added{nullptr};
        Microsoft::UI::Xaml::Style m_removed{nullptr};
        Microsoft::UI::Xaml::Style m_modified{nullptr};
        Microsoft::UI::Xaml::Style m_unchanged{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemItemStyleConverter
        : FilesystemItemStyleConverterT<FilesystemItemStyleConverter, implementation::FilesystemItemStyleConverter>
    {
    };
} // namespace winrt::Flense::factory_implementation
