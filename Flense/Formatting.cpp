#include "pch.h"

#include "Formatting.h"
#if __has_include("Formatting.g.cpp")
#include "Formatting.g.cpp"
#endif

#include "icu.h"

namespace winrt::Flense::implementation
{
    namespace
    {
        struct IcuDateFormatDeleter
        {
            void operator()(URelativeDateTimeFormatter* value) const noexcept
            {
                ureldatefmt_close(value);
            }
        };

    } // namespace

    winrt::hstring Formatting::FormatDate(const Windows::Foundation::DateTime& value)
    {
        static Windows::Globalization::DateTimeFormatting::DateTimeFormatter formatter{L"shortdate shorttime"};

        return formatter.Format(value);
    }

    winrt::hstring Formatting::FormatRelativeTime(const Windows::Foundation::DateTime& value)
    {
        static auto relativeFormatter = []() {
            UErrorCode status = U_ZERO_ERROR;

            // Pass the first arg as nullptr so the user's locale settings are used. Pass the second arg as nullptr to
            // use the default number formatter.
            return std::unique_ptr<URelativeDateTimeFormatter, IcuDateFormatDeleter>{ureldatefmt_open(
                nullptr, nullptr, UDAT_STYLE_LONG, UDISPCTX_CAPITALIZATION_FOR_MIDDLE_OF_SENTENCE, &status)};
        }();

        if (relativeFormatter == nullptr)
        {
            return L"Error";
        }

        auto elapsed = winrt::clock::now() - value;

        double n;
        URelativeDateTimeUnit unit;

        if (auto seconds = std::chrono::round<std::chrono::seconds>(elapsed); seconds < std::chrono::minutes(1))
        {
            n = static_cast<double>(seconds.count());
            unit = UDAT_REL_UNIT_SECOND;
        }
        else if (auto minutes = std::chrono::round<std::chrono::minutes>(elapsed); minutes < std::chrono::hours(1))
        {
            n = minutes.count();
            unit = UDAT_REL_UNIT_MINUTE;
        }
        else if (auto hours = std::chrono::round<std::chrono::hours>(elapsed); hours < std::chrono::days(1))
        {
            n = hours.count();
            unit = UDAT_REL_UNIT_HOUR;
        }
        else if (auto days = std::chrono::round<std::chrono::days>(elapsed); days < std::chrono::weeks(1))
        {
            n = days.count();
            unit = UDAT_REL_UNIT_DAY;
        }
        else if (auto weeks = std::chrono::round<std::chrono::weeks>(elapsed); weeks < std::chrono::months(1))
        {
            n = weeks.count();
            unit = UDAT_REL_UNIT_WEEK;
        }
        else if (auto months = std::chrono::round<std::chrono::months>(elapsed); months < std::chrono::years(1))
        {
            n = months.count();
            unit = UDAT_REL_UNIT_MONTH;
        }
        else
        {
            n = std::chrono::round<std::chrono::years>(elapsed).count();
            unit = UDAT_REL_UNIT_YEAR;
        }

        std::array<UChar, 256> buffer{};
        UErrorCode status = U_ZERO_ERROR;

        std::int32_t length = ureldatefmt_formatNumeric(relativeFormatter.get(), -n, unit, buffer.data(),
                                                        static_cast<int32_t>(buffer.size()), &status);

        if (U_FAILURE(status))
        {
            return L"Error";
        }

        return winrt::hstring(reinterpret_cast<wchar_t*>(buffer.data()), length);
    }

    hstring Formatting::FormatLinkTarget(const hstring& value)
    {
        if (value.empty())
        {
            return value;
        }

        return winrt::hstring(std::format(L"→ {}", value.c_str()));
    }
} // namespace winrt::Flense::implementation
