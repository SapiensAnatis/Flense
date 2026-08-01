#include "pch.h"
#include "AllNotesPage.xaml.h"
#if __has_include("AllNotesPage.g.cpp")
#include "AllNotesPage.g.cpp"
#endif

#include "Models/AllNotes.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    AllNotesPage::AllNotesPage()
    {
		m_notesModel = winrt::make<winrt::Flense::Models::implementation::AllNotes>();
    }

    void AllNotesPage::OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e)
    {
		if (auto note = e.Parameter().try_as<Flense::Models::Note>())
		{
			if (note.State() == Flense::Models::NoteState::Deleted)
			{
				m_notesModel.RemoveNote(note);
			}
            else 
            {
                uint32_t index{};
                if (!m_notesModel.Notes().IndexOf(note, index))
                {
                    m_notesModel.AddNote(note);
                }
            }

            // This navigation should be treated like a back navigation, so clear the backstack.
            Frame().BackStack().Clear();
		}
    }

    winrt::Flense::Models::AllNotes AllNotesPage::NotesModel()
    {
        return m_notesModel;
    }

    void AllNotesPage::NewNoteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
		Frame().Navigate(xaml_typename<Flense::NotePage>());
    }

    void AllNotesPage::ItemsView_ItemInvoked(winrt::Microsoft::UI::Xaml::Controls::ItemsView const& sender, winrt::Microsoft::UI::Xaml::Controls::ItemsViewItemInvokedEventArgs const& args)
    {
        Frame().Navigate(xaml_typename<Flense::NotePage>(), args.InvokedItem());
    }
}




