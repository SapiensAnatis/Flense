#include "pch.h"
#include "ModulePreamble.h"

#include "RatioToPercentStringConverter.h"
#if __has_include("RatioToPercentStringConverter.g.cpp")
#include "RatioToPercentStringConverter.g.cpp"
#endif

#include <format>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml::Interop;

namespace winrt::Flense::implementation
{
    IInspectable RatioToPercentStringConverter::Convert(const IInspectable& value, const TypeName& /* targetType */,
                                                        const IInspectable& /* parameter */,
                                                        const hstring& /* language */)
    {
        const float ratio = value.as<float>();
        return winrt::box_value(winrt::hstring(std::format(L"{:.1f}%", ratio * 100.0f)));
    }

    IInspectable RatioToPercentStringConverter::ConvertBack(const IInspectable& /* value */,
                                                            const TypeName& /* targetType */,
                                                            const IInspectable& /* parameter */,
                                                            const hstring& /* language */)
    {
        // One-way binding only
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
