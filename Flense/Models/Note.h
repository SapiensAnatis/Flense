#pragma once

#include "Models.Note.g.h"

namespace winrt::Flense::Models::implementation
{
	struct Note : NoteT<Note>
	{
	public:
		Note(const winrt::hstring& filename, const winrt::hstring& text, const winrt::Windows::Foundation::DateTime& date, NoteState state = NoteState::Unset)
			: m_filename(filename), m_text(text), m_date(date), m_state(state)
		{
		}

		winrt::Windows::Foundation::IAsyncAction SaveAsync();
		winrt::Windows::Foundation::IAsyncAction DeleteAsync();

		winrt::hstring Text();
		void Text(winrt::hstring const& value);

		winrt::hstring Filename();

		winrt::hstring DisplayDate();

		NoteState State();
		void State(NoteState const& value);

		winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value);
		void PropertyChanged(winrt::event_token const& token);

	private:
		winrt::hstring m_text;
		winrt::hstring m_filename;
		NoteState m_state;
		winrt::Windows::Storage::StorageFolder m_storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
		winrt::Windows::Foundation::DateTime m_date;

		winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
	};
}

namespace winrt::Flense::Models::factory_implementation
{
	struct Note : NoteT<Note, implementation::Note>
	{
	};
}