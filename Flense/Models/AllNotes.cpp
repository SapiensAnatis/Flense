#include "pch.h"
#include "AllNotes.h"
#if __has_include("AllNotes.g.cpp")
#include "AllNotes.g.cpp"
#endif

#include "Models/Note.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;

namespace winrt::Flense::implementation
{
    AllNotes::AllNotes()
    {
		m_notes = winrt::single_threaded_observable_vector<winrt::Flense::Note>();

        LoadNotesAsync();
    }

    IObservableVector<winrt::Flense::Note> AllNotes::Notes()
    {
        return m_notes; 
    }

    winrt::fire_and_forget AllNotes::LoadNotesAsync()
    {
        m_notes.Clear();

        StorageFolder folder = ApplicationData::Current().LocalFolder();

		co_await GetFilesInFolderAsync(folder);
    }

    IAsyncAction AllNotes::GetFilesInFolderAsync(const StorageFolder& folder)
    {
		IVectorView<IStorageItem> storageItems = co_await folder.GetItemsAsync();

		for (const auto& item : storageItems)
		{
			if (item.IsOfType(StorageItemTypes::Folder))
			{
				co_await GetFilesInFolderAsync(item.as<StorageFolder>());
			}
			else if (item.IsOfType(StorageItemTypes::File))
			{
				StorageFile file = item.as<StorageFile>();
				winrt::hstring text = co_await FileIO::ReadTextAsync(file);

				winrt::Flense::Note note = winrt::make<winrt::Flense::implementation::Note>(file.Name(), text);
				m_notes.Append(note);
			}
		}
    }
}
