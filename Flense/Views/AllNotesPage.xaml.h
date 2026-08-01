#pragma once

#include "AllNotesPage.g.h"

#include "Models.AllNotes.g.h"

namespace winrt::Flense::implementation
{
    struct AllNotesPage : AllNotesPageT<AllNotesPage>
    {
        AllNotesPage();

        winrt::Flense::Models::AllNotes NotesModel();

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
