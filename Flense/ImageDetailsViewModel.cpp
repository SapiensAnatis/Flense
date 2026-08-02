#include "pch.h"

#include "ImageDetailsViewModel.h"
#if __has_include("ImageDetailsViewModel.g.cpp")
#include "ImageDetailsViewModel.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml::Data;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;

namespace winrt::Flense::implementation
{
    StorageFile ImageDetailsViewModel::ImageArchive()
    {
        return m_imageFile;
    }

    void ImageDetailsViewModel::ImageArchive(StorageFile const& value)
    {
        if (m_imageFile != value)
        {
            m_imageFile = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"ImageArchive"});
        }
    }

    IObservableVector<hstring> ImageDetailsViewModel::Filenames()
    {
        return m_filenames;
    }

    event_token ImageDetailsViewModel::PropertyChanged(PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void ImageDetailsViewModel::PropertyChanged(event_token const& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
} // namespace winrt::Flense::implementation
