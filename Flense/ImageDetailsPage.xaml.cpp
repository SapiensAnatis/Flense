#include "pch.h"
#include "ImageDetailsPage.xaml.h"
#if __has_include("ImageDetailsPage.g.cpp")
#include "ImageDetailsPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    int32_t ImageDetailsPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void ImageDetailsPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
