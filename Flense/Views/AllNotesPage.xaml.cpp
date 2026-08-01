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




