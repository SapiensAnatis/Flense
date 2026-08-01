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

        int32_t MyProperty();
        void MyProperty(int32_t value);
        void InitializeComponent();
        void AppTitleBar_BackRequested(winrt::Microsoft::UI::Xaml::Controls::TitleBar const& sender, winrt::Windows::Foundation::IInspectable const& args);
    };
}

namespace winrt::Flense::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
