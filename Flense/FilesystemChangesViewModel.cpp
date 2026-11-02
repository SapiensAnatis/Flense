#include "pch.h"

#include "FilesystemChangesViewModel.h"
#if __has_include("FilesystemChangesViewModel.g.cpp")
#include "FilesystemChangesViewModel.g.cpp"
#endif

#include "FilesystemTreeNode.h"

#include <chrono>

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::Flense::implementation
{
    FilesystemChangesViewModel::FilesystemChangesViewModel()
        : INIT_DEBOUNCED_NOTIFYING_PROPERTY(SearchQuery, std::chrono::milliseconds(300), winrt::hstring{})
    {
        SearchQuery.OnCommitted([this] { ApplyFilters(); });
    }

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

    void FilesystemChangesViewModel::ApplyFilters()
    {
        if (!m_nodes)
        {
            return;
        }

        bool noFilteringActive = SearchQuery().empty() && m_changeVisibility.ShowUnchanged &&
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
            get_self<FilesystemTreeNode>(node)->UpdateVisibility(SearchQuery(), m_changeVisibility, false);
        }
    }
} // namespace winrt::Flense::implementation
