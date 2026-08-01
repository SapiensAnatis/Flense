#pragma once

#include "AllNotesPage.g.h"

#include "Models.AllNotes.g.h"

namespace winrt::Flense::implementation
{
    struct AllNotesPage : AllNotesPageT<AllNotesPage>
    {
        AllNotesPage();

        winrt::Flense::Models::AllNotes NotesModel();

        void NewNoteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        winrt::Flense::Models::AllNotes m_notesModel{ nullptr };
    };
}

namespace winrt::Flense::factory_implementation
{
    struct AllNotesPage : AllNotesPageT<AllNotesPage, implementation::AllNotesPage>
    {
    };
}
