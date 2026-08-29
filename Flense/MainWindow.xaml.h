#pragma once

#include "MainWindow.g.h"

namespace winrt::Flense::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void InitializeComponent();

        winrt::Flense::TitleBarService TitleBarService();

        void RootFrame_Navigated(const winrt::Windows::Foundation::IInspectable& sender,
                                 const winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs& e);

        winrt::fire_and_forget AppTitleBar_BackRequested(const winrt::Microsoft::UI::Xaml::Controls::TitleBar& sender,
                                                          const winrt::Windows::Foundation::IInspectable& args);

        void Exit_Click(const winrt::Windows::Foundation::IInspectable& sender,
                        const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
} // namespace winrt::Flense::factory_implementation
