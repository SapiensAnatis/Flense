#include "pch.h"

#include "BindingHelpers.h"
#if __has_include("BindingHelpers.g.cpp")
#include "BindingHelpers.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    bool BindingHelpers::Not(bool value)
    {
        return !value;
    }

    bool BindingHelpers::AndNot(const bool value, const bool negated)
    {
        return value && !negated;
    }
} // namespace winrt::Flense::implementation