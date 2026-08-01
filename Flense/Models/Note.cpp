#include "pch.h"

#include "Note.h"
#include "Note.g.cpp"

using namespace winrt::Windows::Foundation;

namespace winrt::Flense::implementation 
{
	IAsyncAction Note::SaveAsync()
	{
		auto noteFile = (co_await m_storageFolder.TryGetItemAsync(m_filename)).as<winrt::Windows::Storage::IStorageFile>();
		if (!noteFile)
		{
			noteFile = co_await m_storageFolder.CreateFileAsync(m_filename, winrt::Windows::Storage::CreationCollisionOption::ReplaceExisting);
		}

		co_await winrt::Windows::Storage::FileIO::WriteTextAsync(noteFile, m_text);
	}

	IAsyncAction Note::DeleteAsync()
	{
		// Delete the note from the file system.
		auto noteFile = (co_await m_storageFolder.TryGetItemAsync(m_filename)).as<winrt::Windows::Storage::IStorageFile>();
		if (noteFile)
		{
			co_await noteFile.DeleteAsync();
		}
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
			m_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{ L"Text" });
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