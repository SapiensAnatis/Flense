#include "pch.h"

#include "LoadFailureDialog.xaml.h"
#if __has_include("LoadFailureDialog.g.cpp")
#include "LoadFailureDialog.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    winrt::hstring LoadFailureDialog::Message() const
    {
        return m_message;
    }

    void LoadFailureDialog::Message(const winrt::hstring& value)
    {
        m_message = value;
    }

    winrt::hstring LoadFailureDialog::Details() const
    {
        return m_details;
    }

    void LoadFailureDialog::Details(const winrt::hstring& value)
    {
        m_details = value;
    }

    bool LoadFailureDialog::HasDetails() const
    {
        return !m_details.empty();
    }
} // namespace winrt::Flense::implementation
