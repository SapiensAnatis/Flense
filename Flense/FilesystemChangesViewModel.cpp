#include "pch.h"
#include "ModulePreamble.h"

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
            ApplyFilters();
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
            ScheduleApplyFilters();
            m_propertyChanged(*this, PropertyChangedEventArgs{L"SearchQuery"});
        }
    }

    winrt::Flense::FilesystemChangeVisibility FilesystemChangesViewModel::ChangeVisibility()
    {
        return m_changeVisibility;
    }

    void FilesystemChangesViewModel::ChangeVisibility(const winrt::Flense::FilesystemChangeVisibility& value)
    {
        bool changed = m_changeVisibility.ShowUnchanged != value.ShowUnchanged ||
                       m_changeVisibility.ShowAdded != value.ShowAdded ||
                       m_changeVisibility.ShowModified != value.ShowModified ||
                       m_changeVisibility.ShowRemoved != value.ShowRemoved;

        if (changed)
        {
            m_changeVisibility = value;
            ApplyFilters();
            m_propertyChanged(*this, PropertyChangedEventArgs{L"ChangeVisibility"});
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

    event_token FilesystemChangesViewModel::PropertyChanged(const PropertyChangedEventHandler& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void FilesystemChangesViewModel::PropertyChanged(const event_token& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
} // namespace winrt::Flense::implementation
