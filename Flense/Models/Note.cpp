#include "pch.h"
#include "Note.h"
#if __has_include("Models.Note.g.cpp")
#include "Models.Note.g.cpp"
#endif

#include <winrt/Windows.Globalization.DateTimeFormatting.h>

#include <format>

using namespace winrt::Windows::Foundation;

namespace winrt::Flense::Models::implementation 
{
	namespace 
	{
		Windows::Globalization::DateTimeFormatting::DateTimeFormatter fmt{ L"shortdate" };
	}

	IAsyncAction Note::SaveAsync()
	{
		auto noteFile = (co_await m_storageFolder.TryGetItemAsync(m_filename)).as<winrt::Windows::Storage::IStorageFile>();
		if (!noteFile)
		{
			noteFile = co_await m_storageFolder.CreateFileAsync(m_filename, winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);
		}

		co_await winrt::Windows::Storage::FileIO::WriteTextAsync(noteFile, m_text);
		State(NoteState::Saved);
	}

	IAsyncAction Note::DeleteAsync()
	{
		// Delete the note from the file system.
		auto noteFile = (co_await m_storageFolder.TryGetItemAsync(m_filename)).as<winrt::Windows::Storage::IStorageFile>();
		if (noteFile)
		{
			co_await noteFile.DeleteAsync();
		}
		m_filename = L"";
		State(NoteState::Deleted);
	}

	winrt::hstring Note::Text()
	{
		return m_text;
	}

	void Note::Text(winrt::hstring const& value)
	{
		if (m_text != value) 
		{
			m_text = value;
			State(NoteState::Unsaved);
			m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"Text" });
		}
	}

	winrt::hstring implementation::Note::Filename()
	{
		return m_filename;
	}

	winrt::hstring Note::DisplayDate()
	{
		return fmt.Format(m_date);
	}

	NoteState implementation::Note::State()
	{
		return m_state;
	}

	void implementation::Note::State(NoteState const& value)
	{
		if (m_state != value)
		{
			m_state = value;
			m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"State" });
		}
	}

	winrt::event_token Note::PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
	{
		return m_propertyChanged.add(handler);
	}

	void Note::PropertyChanged(winrt::event_token const& token)
	{
		m_propertyChanged.remove(token);
	}
}