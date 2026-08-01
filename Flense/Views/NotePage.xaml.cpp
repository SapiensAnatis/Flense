#include "pch.h"
#include "NotePage.xaml.h"
#if __has_include("NotePage.g.cpp")
#include "NotePage.g.cpp"
#endif

#include "Models/Note.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
	NotePage::NotePage() 
	{
		m_note = winrt::make<::winrt::Flense::Models::implementation::Note>(L"note.txt", L"Note content", winrt::clock::now());
	}

	winrt::fire_and_forget NotePage::SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		co_await m_note.SaveAsync();
	}

	winrt::fire_and_forget NotePage::DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		co_await m_note.DeleteAsync();
	}
}
