#include "pch.h"

#include "AboutDialog.xaml.h"
#if __has_include("AboutDialog.g.cpp")
#include "AboutDialog.g.cpp"
#endif

#include "AppVersion.h"
#include "License.h"

import std;

using namespace winrt::Windows::Storage;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::Flense::implementation
{
    namespace
    {
        std::wstring_view TrimLeadingTrailingNewlines(std::wstring_view text)
        {
            const auto first = text.find_first_not_of(L'\n');
            if (first == std::wstring_view::npos)
            {
                return {};
            }

            const auto last = text.find_last_not_of(L'\n');
            return text.substr(first, last - first + 1);
        }
    } // namespace

    void AboutDialog::InitializeComponent()
    {
        AboutDialogT::InitializeComponent();
        LoadLicensesAsync();
    }

    hstring AboutDialog::AppVersion()
    {
        return GetAppVersionString();
    }

    winrt::fire_and_forget AboutDialog::LoadLicensesAsync()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue::GetForCurrentThread();

        std::optional<winrt::hstring> failure;
        std::vector<Flense::License> licenses;

        co_await winrt::resume_background();

        try
        {
            const StorageFile file = co_await StorageFile::GetFileFromApplicationUriAsync(
                Windows::Foundation::Uri(L"ms-appx:///THIRD-PARTY-NOTICES.md"));

            const IVector<hstring> lines = co_await FileIO::ReadLinesAsync(file);

            std::wstring currentName;
            std::wstring currentNotice;

            const auto flush = [&] {
                if (!currentName.empty() && !currentNotice.empty())
                {
                    licenses.push_back(winrt::make<License>(
                        winrt::hstring(currentName), winrt::hstring(TrimLeadingTrailingNewlines(currentNotice))));
                }
            };

            for (const winrt::hstring& line : lines)
            {
                static constexpr std::wstring_view LibraryHeadingPrefix = L"# ";

                const std::wstring_view lineView{line};

                if (lineView.starts_with(LibraryHeadingPrefix))
                {
                    flush();

                    currentName = lineView.substr(LibraryHeadingPrefix.size());
                    currentNotice = L"";
                }
                else
                {
                    currentNotice += lineView;
                    currentNotice += '\n';
                }
            }

            flush();
        }
        catch (const winrt::hresult_error& ex)
        {
            // Can't co_await in catch blocks
            failure = ex.message();
        }
        catch (...)
        {
            failure = L"Unknown error";
        }

        co_await wil::resume_foreground(dispatcher);

        if (failure)
        {
            LicenseErrorText().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Visible);
            LicenseErrorText().Text(std::format(L"Failed to load license information: {}", *failure));
        }
        else
        {
            Licenses().ReplaceAll(licenses);
        }
    }
} // namespace winrt::Flense::implementation
