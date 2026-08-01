#pragma once

#include "AllNotes.g.h"

namespace winrt::Flense::implementation
{
    struct AllNotes : AllNotesT<AllNotes>
    {
        AllNotes();

		winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::Note> Notes();

    private:
		winrt::fire_and_forget LoadNotesAsync();
        winrt::Windows::Foundation::IAsyncAction GetFilesInFolderAsync(const winrt::Windows::Storage::StorageFolder& folder);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::Note> m_notes;
    };
}

namespace winrt::Flense::factory_implementation
{
    struct AllNotes : AllNotesT<AllNotes, implementation::AllNotes>
    {
    };
}
