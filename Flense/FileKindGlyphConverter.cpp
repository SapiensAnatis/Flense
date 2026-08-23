#include "pch.h"

#include "FileKindGlyphConverter.h"
#if __has_include("FileKindGlyphConverter.g.cpp")
#include "FileKindGlyphConverter.g.cpp"
#endif

#include "FileKind.h"
#include "winrt/Flense.h"

using namespace winrt::Windows::Foundation;
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

    IInspectable FileKindGlyphConverter::Convert(const IInspectable& value, const TypeName& /* targetType */,
                                                 const IInspectable& /* parameter */, const hstring& /* language */)
    {
        static const IInspectable folderGlyph = winrt::box_value(winrt::hstring(FolderGlyph));
        static const IInspectable linkGlyph = winrt::box_value(winrt::hstring(LinkGlyph));
        static const IInspectable documentGlyph = winrt::box_value(winrt::hstring(DocumentGlyph));

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

    IInspectable FileKindGlyphConverter::ConvertBack(const IInspectable& /* value */, const TypeName& /* targetType */,
                                                     const IInspectable& /* parameter */, const hstring& /* language */)
    {
        // One-way binding only
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
