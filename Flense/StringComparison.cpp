#include "pch.h"

#include "StringComparison.h"

#include <type_traits>

namespace winrt::Flense::implementation
{
    namespace
    {
        template <typename T>
        concept WString = requires(T string) {
            { string.data() } -> std::convertible_to<const wchar_t*>;
            { string.size() } -> std::convertible_to<int>;
        };

        template <WString T, WString U> bool ContainsCaseInsensitiveImpl(const T& haystack, const U& needle)
        {
            if (haystack.size() == 0 && needle.size() > 0)
            {
                return false;
            }

            if (needle.size() == 0)
            {
                return true;
            }

            if (FindNLSStringEx(LOCALE_NAME_USER_DEFAULT, FIND_FROMSTART | LINGUISTIC_IGNORECASE, haystack.data(),
                                static_cast<int>(haystack.size()), needle.data(), static_cast<int>(needle.size()),
                                nullptr, nullptr, nullptr, 0) == -1)
            {
                // "ERROR_SUCCESS. The action completed successfully but yielded no results."
                assert(GetLastError() == ERROR_SUCCESS);
                return false;
            }

            return true;
        }
    } // namespace

    bool ContainsCaseInsensitive(const winrt::hstring& haystack, const winrt::hstring& needle)
    {
        return ContainsCaseInsensitiveImpl(haystack, needle);
    }

    bool ContainsCaseInsensitive(const winrt::hstring& haystack, const std::wstring_view& needle)
    {
        return ContainsCaseInsensitiveImpl(haystack, needle);
    }
} // namespace winrt::Flense::implementation