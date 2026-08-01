#pragma once

#include "Note.g.h"

namespace winrt::Flense::implementation
{
	struct Note : NoteT<Note>
	{
	public:
		Note() 
		{
			m_filename =  L"notes" + std::to_wstring(m_date.time_since_epoch().count()) + L".txt";
		}

		winrt::Windows::Foundation::IAsyncAction SaveAsync();
		winrt::Windows::Foundation::IAsyncAction DeleteAsync();

		
		winrt::hstring Text();
		void Text(winrt::hstring const& value);

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

namespace winrt::Flense::factory_implementation
{
	struct Note : NoteT<Note, implementation::Note>
	{
	};
}