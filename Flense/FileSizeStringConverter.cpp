#include "pch.h"

#include "FileSizeStringConverter.h"
#if __has_include("FileSizeStringConverter.g.cpp")
#include "FileSizeStringConverter.g.cpp"
#endif

#include <format>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::UI::Xaml::Interop;

namespace winrt::Flense::implementation
{
    static constexpr uint64_t OneTerabyte = static_cast<uint64_t>(1e12);
    static constexpr uint64_t OneGigabyte = static_cast<uint64_t>(1e9);
    static constexpr uint64_t OneMegabyte = static_cast<uint64_t>(1e6);
    static constexpr uint64_t OneKilobyte = static_cast<uint64_t>(1e3);

    IInspectable FileSizeStringConverter::Convert(const IInspectable& value, const TypeName& /* targetType */,
                                                  const IInspectable& /* parameter */, const hstring& /* language */)
    {
        const uint64_t castedValue = value.as<uint64_t>();

        std::wstring_view siPrefix = L"";
        uint64_t divisor = 1;

        if (castedValue >= OneTerabyte)
        {
            siPrefix = L"T";
            divisor = OneTerabyte;
        }
        else if (castedValue >= OneGigabyte)
        {
            siPrefix = L"G";
            divisor = OneGigabyte;
        }
        else if (castedValue >= OneMegabyte)
        {
            siPrefix = L"M";
            divisor = OneMegabyte;
        }
        else if (castedValue >= OneKilobyte)
        {
            siPrefix = L"K";
            divisor = OneKilobyte;
        }

        winrt::hstring str;

        if (!siPrefix.empty())
        {
            const double fractionalValue = castedValue / static_cast<double>(divisor);
            str = winrt::hstring(std::format(L"{:.1f} {}B", fractionalValue, siPrefix));
        }
        else
        {
            str = winrt::hstring(std::format(L"{} B", castedValue));
        }

        return winrt::box_value(str);
    }

    IInspectable FileSizeStringConverter::ConvertBack(const IInspectable& /* value */, const TypeName& /* targetType */,
                                                      const IInspectable& /* parameter */,
                                                      const hstring& /* language */)
    {
        // One-way binding only
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
