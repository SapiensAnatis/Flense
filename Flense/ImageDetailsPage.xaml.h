#pragma once

#include "ImageDetailsPage.g.h"

namespace winrt::Flense::implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage>
    {
        ImageDetailsPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage, implementation::ImageDetailsPage>
    {
    };
}
