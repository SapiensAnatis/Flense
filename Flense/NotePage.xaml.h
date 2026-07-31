#pragma once

#include "NotePage.g.h"

namespace winrt::Flense::implementation
{
    struct NotePage : NotePageT<NotePage>
    {
        NotePage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void InitializeComponent();
        winrt::fire_and_forget Loaded(
            winrt::Windows::Foundation::IInspectable const& sender,
            winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:

		Windows::Storage::StorageFolder m_storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
        Windows::Storage::IStorageFile m_noteFile{ nullptr };
        winrt::hstring m_fileName{ L"note.txt" };
    public:
        winrt::fire_and_forget SaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::fire_and_forget DeleteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::Flense::factory_implementation
{
    struct NotePage : NotePageT<NotePage, implementation::NotePage>
    {
    };
}
