#include "pch.h"

#include "FilesystemItemColourConverter.h"
#if __has_include("FilesystemItemColourConverter.g.cpp")
#include "FilesystemItemColourConverter.g.cpp"
#endif

using namespace winrt::Windows::UI::Xaml::Interop;

#include "winrt/Flense.h"

namespace winrt::Flense::implementation
{
    /// <remarks>
    /// The styles are supplied from XAML and set their background with {ThemeResource}, so the framework re-resolves
    /// the colours when the theme changes.
    /// </remarks>
    winrt::Windows::Foundation::IInspectable FilesystemItemColourConverter::Convert(
        const winrt::Windows::Foundation::IInspectable& value, const TypeName& /* targetType */,
        const winrt::Windows::Foundation::IInspectable& /* parameter */, const hstring& /* language */)
    {
        const auto kind = value.as<Flense::FilesystemChangeKind>();

        using enum Flense::FilesystemChangeKind;

        switch (kind)
        {
        case Added:
            return m_addedStyle;
        case Removed:
            return m_removedStyle;
        case Modified:
            return m_modifiedStyle;
        default:
            return nullptr;
        }
    }

    winrt::Windows::Foundation::IInspectable FilesystemItemColourConverter::ConvertBack(
        const winrt::Windows::Foundation::IInspectable& /* value */, const TypeName& /* targetType */,
        const winrt::Windows::Foundation::IInspectable& /* parameter */, const hstring& /* language */)
    {
        // One-way binding only
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
