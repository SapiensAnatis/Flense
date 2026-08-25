#include "pch.h"

#include "FilesystemChangesViewModel.h"
#if __has_include("FilesystemChangesViewModel.g.cpp")
#include "FilesystemChangesViewModel.g.cpp"
#endif

#include "FilesystemTreeNode.h"

#include <chrono>

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml::Data;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::Flense::implementation
{
    IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemChangesViewModel::Nodes()
    {
        return m_nodes;
    }

    void FilesystemChangesViewModel::Nodes(const IObservableVector<winrt::Flense::FilesystemTreeNode>& value)
    {
        if (m_nodes != value)
        {
            m_nodes = value;
            ApplySearchQuery();
            m_propertyChanged(*this, PropertyChangedEventArgs{L"Nodes"});
        }
    }

    hstring FilesystemChangesViewModel::SearchQuery()
    {
        return m_searchQuery;
    }

    void FilesystemChangesViewModel::SearchQuery(const hstring& value)
    {
        if (m_searchQuery != value)
        {
            m_searchQuery = value;
            ScheduleApplySearchQuery();
            m_propertyChanged(*this, PropertyChangedEventArgs{L"SearchQuery"});
        }
    }

    void FilesystemChangesViewModel::ScheduleApplySearchQuery()
    {
        static constexpr auto SearchDebounceTimeSpan = TimeSpan{std::chrono::milliseconds(300)};

        if (!m_searchDebounceTimer)
        {
            m_searchDebounceTimer = DispatcherQueue::GetForCurrentThread().CreateTimer();
            m_searchDebounceTimer.Interval(SearchDebounceTimeSpan);
            m_searchDebounceTimer.IsRepeating(false);

            auto weakThis = get_weak();
            m_searchDebounceTimer.Tick([weakThis](auto&&, auto&&) {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->ApplySearchQuery();
                }
            });
        }

        m_searchDebounceTimer.Stop();
        m_searchDebounceTimer.Start();
    }

    void FilesystemChangesViewModel::ApplySearchQuery()
    {
        if (!m_nodes)
        {
            return;
        }

        for (const auto& node : m_nodes)
        {
            get_self<FilesystemTreeNode>(node)->UpdateVisibility(m_searchQuery);
        }
    }

    event_token FilesystemChangesViewModel::PropertyChanged(const PropertyChangedEventHandler& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void FilesystemChangesViewModel::PropertyChanged(const event_token& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
} // namespace winrt::Flense::implementation
