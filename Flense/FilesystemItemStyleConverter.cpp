#include "pch.h"

#include "FilesystemItemStyleConverter.h"
#if __has_include("FilesystemItemStyleConverter.g.cpp")
#include "FilesystemItemStyleConverter.g.cpp"
#endif

#include "winrt/Flense.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml::Interop;

namespace winrt::Flense::implementation
{
    IInspectable FilesystemItemStyleConverter::Convert(const IInspectable& value, const TypeName& /* targetType */,
                                                       const IInspectable& /* parameter */,
                                                       const hstring& /* language */)
    {
        const auto kind = value.as<Flense::FilesystemChangeKind>();

        switch (kind)
        {
        case Flense::FilesystemChangeKind::Added:
            return m_added;
        case Flense::FilesystemChangeKind::Removed:
            return m_removed;
        case Flense::FilesystemChangeKind::Modified:
            return m_modified;
        default:
            return m_unchanged;
        }
    }

    IInspectable FilesystemItemStyleConverter::ConvertBack(const IInspectable& /* value */,
                                                           const TypeName& /* targetType */,
                                                           const IInspectable& /* parameter */,
                                                           const hstring& /* language */)
    {
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
