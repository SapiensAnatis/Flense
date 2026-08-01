#pragma once

#include "Models.Note.g.h"

namespace winrt::Flense::Models::implementation
{
	struct Note : NoteT<Note>
	{
	public:
		Note(const winrt::hstring& filename, const winrt::hstring& text)
		{
			m_filename = filename;
			m_text = text;
		}

		winrt::Windows::Foundation::IAsyncAction SaveAsync();
		winrt::Windows::Foundation::IAsyncAction DeleteAsync();

		winrt::hstring Text();
		void Text(winrt::hstring const& value);

		winrt::hstring DisplayDate();

		winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value);
		void PropertyChanged(winrt::event_token const& token);

	private:
		winrt::hstring m_filename;
		winrt::hstring m_text;
		winrt::Windows::Storage::StorageFolder m_storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
		winrt::Windows::Foundation::DateTime m_date{ winrt::clock::now() };

		winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
	};
}

namespace winrt::Flense::Models::factory_implementation
{
	struct Note : NoteT<Note, implementation::Note>
	{
	};
}