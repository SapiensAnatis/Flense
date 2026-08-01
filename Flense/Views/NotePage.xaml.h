#pragma once

#include "NotePage.g.h"

#include "Models.Note.g.h"

namespace winrt::Flense::implementation
{
    struct NotePage : NotePageT<NotePage>
    {
        NotePage();

		void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);
		winrt::fire_and_forget OnNavigatingFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs const& e);

        winrt::fire_and_forget SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

        Flense::Models::Note Note()
        {
            return m_note;
        }

    private:
        Flense::Models::Note m_note{ nullptr };
    };
}

namespace winrt::Flense::factory_implementation
{
    struct NotePage : NotePageT<NotePage, implementation::NotePage>
    {
    };
}
