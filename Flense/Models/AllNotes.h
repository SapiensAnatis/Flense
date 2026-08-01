#pragma once

#include "Models.AllNotes.g.h"

namespace winrt::Flense::Models::implementation
{
    struct AllNotes : AllNotesT<AllNotes>
    {
        AllNotes();

		winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::Models::Note> Notes();

        void AddNote(const winrt::Flense::Models::Note& note);
		void RemoveNote(const winrt::Flense::Models::Note& note);

    private:
		winrt::fire_and_forget LoadNotesAsync();
        winrt::Windows::Foundation::IAsyncAction GetFilesInFolderAsync(const winrt::Windows::Storage::StorageFolder& folder);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::Models::Note> m_notes;
    };
}

namespace winrt::Flense::Models::factory_implementation
{
    struct AllNotes : AllNotesT<AllNotes, implementation::AllNotes>
    {
    };
}
