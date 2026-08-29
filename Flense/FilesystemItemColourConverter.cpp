#include "pch.h"
#include "ModulePreamble.h"

#include "FilesystemItemColourConverter.h"
#if __has_include("FilesystemItemColourConverter.g.cpp")
#include "FilesystemItemColourConverter.g.cpp"
#endif

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Interop;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace winrt::Flense::implementation
{
    namespace
    {
        static SolidColorBrush AddedBrush{nullptr};
        static SolidColorBrush RemovedBrush{nullptr};
        static SolidColorBrush ModifiedBrush{nullptr};
        static SolidColorBrush DefaultBrush{nullptr};
    } // namespace

    void FilesystemItemColourConverter::InitializeComponent()
    {
        auto resources = Application::Current().Resources();

        auto successBrush = resources.Lookup(winrt::box_value(L"SystemFillColorSuccessBrush")).as<SolidColorBrush>();
        AddedBrush = SolidColorBrush(successBrush.Color());
        AddedBrush.Opacity(0.2);

        auto critcalBrush = resources.Lookup(winrt::box_value(L"SystemFillColorCriticalBrush")).as<SolidColorBrush>();
        RemovedBrush = SolidColorBrush(critcalBrush.Color());
        RemovedBrush.Opacity(0.2);

        auto cautionBrush = resources.Lookup(winrt::box_value(L"SystemFillColorCautionBrush")).as<SolidColorBrush>();
        ModifiedBrush = SolidColorBrush(cautionBrush.Color());
        ModifiedBrush.Opacity(0.2);

        DefaultBrush = SolidColorBrush(winrt::Windows::UI::Color());
    }

    IInspectable FilesystemItemColourConverter::Convert(const IInspectable& value, const TypeName& /* targetType */,
                                                        const IInspectable& /* parameter */,
                                                        const hstring& /* language */)
    {
        const auto kind = value.as<Flense::FilesystemChangeKind>();

        using enum Flense::FilesystemChangeKind;

        switch (kind)
        {
        case Added:
            return AddedBrush;
        case Removed:
            return RemovedBrush;
        case Modified:
            return ModifiedBrush;
        default:
            return DefaultBrush;
        }
    }

    IInspectable FilesystemItemColourConverter::ConvertBack(const IInspectable& /* value */,
                                                            const TypeName& /* targetType */,
                                                            const IInspectable& /* parameter */,
                                                            const hstring& /* language */)
    {
        throw hresult_not_implemented();
    }
} // namespace winrt::Flense::implementation
