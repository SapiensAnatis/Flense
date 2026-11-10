#pragma once

#include "AboutDialog.g.h"

namespace winrt::Flense::implementation
{
    struct AboutDialog : AboutDialogT<AboutDialog>
    {
        AboutDialog() : Licenses(winrt::single_threaded_observable_vector<winrt::Flense::License>())
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void InitializeComponent();

        hstring AppVersion();

        wil::single_threaded_property<
            winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::License>>
            Licenses;

      private:
        winrt::fire_and_forget LoadLicensesAsync();
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct AboutDialog : AboutDialogT<AboutDialog, implementation::AboutDialog>
    {
    };
} // namespace winrt::Flense::factory_implementation
