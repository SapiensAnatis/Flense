#pragma once

#include "AllNotesPage.g.h"

#include "Models/AllNotes.h"

namespace winrt::Flense::implementation
{
    struct AllNotesPage : AllNotesPageT<AllNotesPage>
    {
        AllNotesPage();

        winrt::Flense::AllNotes NotesModel();

    private:
        winrt::Flense::AllNotes m_notesModel{ nullptr };
    };
}

namespace winrt::Flense::factory_implementation
{
    struct AllNotesPage : AllNotesPageT<AllNotesPage, implementation::AllNotesPage>
    {
    };
}
