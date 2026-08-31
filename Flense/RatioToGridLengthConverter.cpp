#include "pch.h"

#include "RatioToGridLengthConverter.h"
#if __has_include("RatioToGridLengthConverter.g.cpp")
#include "RatioToGridLengthConverter.g.cpp"
#endif

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml::Interop;
using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::Flense::implementation
{
    /// <remarks>
    /// Pass ConverterParameter="Invert" to get the remaining (1 - ratio) share, e.g. for a spacer column that sits
    /// alongside a column using the un-inverted ratio, so the two always sum to the full available width.
    /// </remarks>
    IInspectable RatioToGridLengthConverter::Convert(const IInspectable& value, const TypeName& /* targetType */,
                                                      const IInspectable& parameter, const hstring& /* language */)
    {
        float ratio = value.as<float>();

        if (parameter && winrt::unbox_value_or<hstring>(parameter, L"") == L"Invert")
        {
            ratio = 1.0f - ratio;
        }

        return winrt::box_value(GridLength{.Value = ratio, .GridUnitType = GridUnitType::Star});
    }

    IInspectable RatioToGridLengthConverter::ConvertBack(const IInspectable& /* value */,
                                                          const TypeName& /* targetType */,
                                                          const IInspectable& /* parameter */,
                                                          const hstring& /* language */)
    {
        // One-way binding only
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
