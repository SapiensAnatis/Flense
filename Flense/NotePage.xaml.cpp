#include "pch.h"
#include <iostream>
#include "NotePage.xaml.h"
#if __has_include("NotePage.g.cpp")
#include "NotePage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
	void NotePage::InitializeComponent()
	{
		NotePageT::InitializeComponent();
	}

	winrt::fire_and_forget NotePage::Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
	{
		m_noteFile = (co_await m_storageFolder.TryGetItemAsync(m_fileName)).as<Windows::Storage::IStorageFile>();
		if (m_noteFile) 
		{
			winrt::hstring text = co_await Windows::Storage::FileIO::ReadTextAsync(m_noteFile);
			NoteEditor().Text(text);
		}
	}
}

winrt::fire_and_forget winrt::Flense::implementation::NotePage::SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
{
	if (!m_noteFile) 
	{
		m_noteFile = co_await m_storageFolder.CreateFileAsync(m_fileName, Windows::Storage::CreationCollisionOption::ReplaceExisting);
	}

	co_await winrt::Windows::Storage::FileIO::WriteTextAsync(m_noteFile, NoteEditor().Text());
}

winrt::fire_and_forget winrt::Flense::implementation::NotePage::DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
{
	if (m_noteFile) 
	{
		co_await m_noteFile.DeleteAsync();
		m_noteFile = nullptr;
		NoteEditor().Text(L"");
	}
}
