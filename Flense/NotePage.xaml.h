#pragma once

#include "NotePage.g.h"

#include "Note.h"

namespace winrt::Flense::implementation
{
    struct NotePage : NotePageT<NotePage>
    {
        NotePage();

        winrt::fire_and_forget SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        Flense::Note Note()
        {
            return m_note;
        }

    private:
        Flense::Note m_note{ nullptr };
    };
}

namespace winrt::Flense::factory_implementation
{
    struct NotePage : NotePageT<NotePage, implementation::NotePage>
    {
    };
}
