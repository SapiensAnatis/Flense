#include "pch.h"
#include "NotePage.xaml.h"
#if __has_include("NotePage.g.cpp")
#include "NotePage.g.cpp"
#endif

#include "Models/Note.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
	NotePage::NotePage() 
	{
	}

	void NotePage::OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e)
	{
		NotePageT::OnNavigatedTo(e);

		if (auto note = e.Parameter().try_as<Flense::Models::Note>())
		{
			m_note = note;
		}
		else 
		{
			long long msEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(winrt::clock::now().time_since_epoch()).count();
			std::wstring name = L"note-" + std::to_wstring(msEpoch) + L".txt";

			m_note = winrt::make<::winrt::Flense::Models::implementation::Note>(winrt::hstring(name), L"", winrt::clock::now());
		}
	}

	winrt::fire_and_forget NotePage::OnNavigatingFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs const& e)
	{
		if (m_note && m_note.State() == Flense::Models::NoteState::Unsaved)
		{
			e.Cancel(true);
			ContentDialog dialog;

			dialog.XamlRoot(this->XamlRoot());
			dialog.Title(winrt::box_value(L"Save your work?"));
			dialog.PrimaryButtonText(winrt::to_hstring(L"Save"));
			dialog.SecondaryButtonText(winrt::to_hstring(L"Don't Save"));
			dialog.CloseButtonText(winrt::to_hstring(L"Cancel"));
			dialog.DefaultButton(ContentDialogButton::Primary);

			ContentDialogResult result = co_await dialog.ShowAsync();

			if (result == ContentDialogResult::Primary)
			{
				co_await m_note.SaveAsync();
				Frame().Navigate(xaml_typename<Flense::AllNotesPage>(), m_note);
			}
			else if (result == ContentDialogResult::Secondary)
			{
				while (NoteEditor().CanUndo())
				{
					NoteEditor().Undo();
				}
				NoteEditor().Focus(FocusState::Programmatic);
				m_note.State(Flense::Models::NoteState::Saved);
				Frame().Navigate(xaml_typename<Flense::AllNotesPage>(), m_note);
			}
		}
	}

	winrt::fire_and_forget NotePage::SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		if (m_note)
		{
			co_await m_note.SaveAsync();
		}
	}

	winrt::fire_and_forget NotePage::DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
	{
		if (m_note)
		{
			co_await m_note.DeleteAsync();
		}

		if (Frame().CanGoBack()) 
		{
			Frame().GoBack();
		}
	}
}
