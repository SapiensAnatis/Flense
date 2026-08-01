#include "pch.h"
#include "AllNotesPage.xaml.h"
#if __has_include("AllNotesPage.g.cpp")
#include "AllNotesPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    AllNotesPage::AllNotesPage()
    {
		m_notesModel = winrt::make<winrt::Flense::implementation::AllNotes>();
    }

    winrt::Flense::AllNotes AllNotesPage::NotesModel()
    {
        return m_notesModel;
    }
}
