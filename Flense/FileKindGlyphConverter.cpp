#include "pch.h"

#include "FileKindGlyphConverter.h"
#if __has_include("FileKindGlyphConverter.g.cpp")
#include "FileKindGlyphConverter.g.cpp"
#endif

#include "winrt/Flense.h"

import Flense.Core;

using namespace winrt::Windows::UI::Xaml::Interop;

namespace winrt::Flense::implementation
{
    namespace
    {
        // Segoe Fluent Icons codepoints.
        constexpr std::wstring_view FolderGlyph = L"\uE8B7";   // FolderHorizontal
        constexpr std::wstring_view LinkGlyph = L"\uE71B";     // Link
        constexpr std::wstring_view DocumentGlyph = L"\uE8A5"; // Document
    } // namespace

    winrt::Windows::Foundation::IInspectable FileKindGlyphConverter::Convert(
        const winrt::Windows::Foundation::IInspectable& value, const TypeName& /* targetType */,
        const winrt::Windows::Foundation::IInspectable& /* parameter */, const hstring& /* language */)
    {
        static const winrt::Windows::Foundation::IInspectable folderGlyph =
            winrt::box_value(winrt::hstring(FolderGlyph));
        static const winrt::Windows::Foundation::IInspectable linkGlyph = winrt::box_value(winrt::hstring(LinkGlyph));
        static const winrt::Windows::Foundation::IInspectable documentGlyph =
            winrt::box_value(winrt::hstring(DocumentGlyph));

        const auto kind = value.as<Flense::FileKind>();

        using enum Flense::FileKind;

        switch (kind)
        {
        case Directory:
            return folderGlyph;
        case Symlink:
            return linkGlyph;
        default:
            return documentGlyph;
        }
    }

    winrt::Windows::Foundation::IInspectable FileKindGlyphConverter::ConvertBack(
        const winrt::Windows::Foundation::IInspectable& /* value */, const TypeName& /* targetType */,
        const winrt::Windows::Foundation::IInspectable& /* parameter */, const hstring& /* language */)
    {
        // One-way binding only
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
