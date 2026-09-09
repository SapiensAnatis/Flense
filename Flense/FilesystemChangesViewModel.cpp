#include "pch.h"

#include "FilesystemChangesViewModel.h"
#if __has_include("FilesystemChangesViewModel.g.cpp")
#include "FilesystemChangesViewModel.g.cpp"
#endif

#include "FilesystemTreeNode.h"

#include <chrono>

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
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
            ApplyFilters();
            RaisePropertyChanged(L"Nodes");
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
            ScheduleApplyFilters();
            RaisePropertyChanged(L"SearchQuery");
        }
    }

    winrt::Flense::FilesystemChangeVisibility FilesystemChangesViewModel::ChangeVisibility()
    {
        return m_changeVisibility;
    }

    void FilesystemChangesViewModel::ChangeVisibility(const winrt::Flense::FilesystemChangeVisibility& value)
    {
        if (m_changeVisibility != value)
        {
            m_changeVisibility = value;
            ApplyFilters();
            RaisePropertyChanged(L"ChangeVisibility");
        }
    }

    void FilesystemChangesViewModel::ScheduleApplyFilters()
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
                    strongThis->ApplyFilters();
                }
            });
        }

        m_searchDebounceTimer.Stop();
        m_searchDebounceTimer.Start();
    }

    void FilesystemChangesViewModel::ApplyFilters()
    {
        if (!m_nodes)
        {
            return;
        }

        bool noFilteringActive = m_searchQuery.empty() && m_changeVisibility.ShowUnchanged &&
                                 m_changeVisibility.ShowAdded && m_changeVisibility.ShowModified &&
                                 m_changeVisibility.ShowRemoved;

        // Early returning if noFilteringActive has a bug where clearing your filters won't make all nodes visible
        // again.
        if (noFilteringActive)
        {
            // Make all realised children visible again
            auto makeVisible = [](const winrt::Flense::FilesystemTreeNode& node, auto&& makeVisible) -> void {
                if (node.IsExpanded())
                {
                    for (const auto& child : node.Children())
                    {
                        makeVisible(child, makeVisible);
                    }
                }

                get_self<FilesystemTreeNode>(node)->Visible(true);
            };

            for (const auto& node : m_nodes)
            {
                makeVisible(node, makeVisible);
            }
        }

        for (const auto& node : m_nodes)
        {
            get_self<FilesystemTreeNode>(node)->UpdateVisibility(m_searchQuery, m_changeVisibility, false);
        }
    }
} // namespace winrt::Flense::implementation
