#include "pch.h"

#include "FilesystemTreeNode.h"
#if __has_include("FilesystemTreeNode.g.cpp")
#include "FilesystemTreeNode.g.cpp"
#endif

using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Microsoft::UI::Xaml::Data;

namespace winrt::Flense::implementation
{
    FilesystemTreeNode::FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node)
        : m_name(std::move(name)), m_node(std::move(node))
    {
    }

    FilesystemTreeNode::~FilesystemTreeNode()
    {
    }

    winrt::hstring FilesystemTreeNode::Name()
    {
        return m_name;
    }

    winrt::Flense::FileKind FilesystemTreeNode::Kind()
    {
        switch (m_node->Data().kind)
        {
        case ::Flense::Core::FileKind::File:
            return winrt::Flense::FileKind::File;
        case ::Flense::Core::FileKind::Directory:
            return winrt::Flense::FileKind::Directory;
        case ::Flense::Core::FileKind::Symlink:
            return winrt::Flense::FileKind::Symlink;
        case ::Flense::Core::FileKind::Other:
            return winrt::Flense::FileKind::Other;
        default:
            return winrt::Flense::FileKind::Unspecified;
        }
    }

    winrt::Flense::FilesystemChangeKind FilesystemTreeNode::ChangeKind()
    {
        switch (m_node->Data().changeKind)
        {
        case ::Flense::Core::FilesystemChangeKind::None:
            return winrt::Flense::FilesystemChangeKind::None;
        case ::Flense::Core::FilesystemChangeKind::Added:
            return winrt::Flense::FilesystemChangeKind::Added;
        case ::Flense::Core::FilesystemChangeKind::Removed:
            return winrt::Flense::FilesystemChangeKind::Removed;
        case ::Flense::Core::FilesystemChangeKind::Modified:
            return winrt::Flense::FilesystemChangeKind::Modified;
        default:
            return winrt::Flense::FilesystemChangeKind::Unspecified;
        }
    }

    uint64_t FilesystemTreeNode::Size()
    {
        return m_node->Data().size;
    }

    bool FilesystemTreeNode::Visible()
    {
        return m_visible;
    }

    /// <remarks>Not exposed via IDL, intended for internal use only</remarks>
    void FilesystemTreeNode::Visible(bool value)
    {
        if (m_visible != value)
        {
            m_visible = value;
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{L"Visible"});
        }
    }

    bool FilesystemTreeNode::IsExpanded()
    {
        return m_isExpanded;
    }

    void FilesystemTreeNode::IsExpanded(bool value)
    {
        if (m_isExpanded != value)
        {
            m_isExpanded = value;
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{L"IsExpanded"});
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{L"ChildrenIfExpanded"});
        }
    }

    IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemTreeNode::Children()
    {
        if (!m_children)
        {
            std::vector<winrt::Flense::FilesystemTreeNode> children;
            children.reserve(m_node->Children().size());

            for (const auto& [name, child] : m_node->Children())
            {
                children.push_back(winrt::make<FilesystemTreeNode>(winrt::to_hstring(name), child));
            }

            m_children =
                winrt::single_threaded_observable_vector<winrt::Flense::FilesystemTreeNode>(std::move(children));
        }

        return m_children;
    }

    IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemTreeNode::ChildrenIfExpanded()
    {
        if (!m_isExpanded && !m_children)
        {
            return nullptr;
        }

        return Children();
    }

    bool FilesystemTreeNode::HasChildren()
    {
        return !m_node->Children().empty();
    }

    winrt::event_token FilesystemTreeNode::PropertyChanged(const PropertyChangedEventHandler& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void FilesystemTreeNode::PropertyChanged(const winrt::event_token& token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    bool FilesystemTreeNode::MatchesChangeKindFilter(const winrt::Flense::FilesystemChangeVisibility& filter)
    {
        switch (ChangeKind())
        {
        case winrt::Flense::FilesystemChangeKind::Added:
            return filter.ShowAdded;
        case winrt::Flense::FilesystemChangeKind::Removed:
            return filter.ShowRemoved;
        case winrt::Flense::FilesystemChangeKind::Modified:
            return filter.ShowModified;
        default:
            return filter.ShowUnchanged;
        }
    }

    /// <summary>
    /// Recomputes visibility for this node and its descendants against a search query and a change-kind filter. A
    /// node is visible if its name matches the query and its own change kind is enabled in the filter, or if it is
    /// a descendant of any directory that matches the query, or if any descendant is visible.
    /// </summary>
    /// <returns>
    /// A boolean indicating whether this node was determined to be visible.
    /// </returns>
    bool FilesystemTreeNode::UpdateVisibility(std::wstring_view query, const winrt::Flense::FilesystemChangeVisibility& filter,
                                              bool parentMatches)
    {
        // TODO: This causes the entire tree to be materialized by calling Children() on every node, we could find a
        // way to improve this, e.g. searching over the core tree type and materializing only matching nodes.
        bool anyChildVisible = false;

        bool thisNodeMatch = std::wstring_view{m_name}.contains(query);

        for (const auto& child : Children())
        {
            anyChildVisible |= winrt::get_self<FilesystemTreeNode>(child)->UpdateVisibility(
                query, filter, parentMatches || thisNodeMatch);
        }

        bool searchVisible = parentMatches || query.empty() || thisNodeMatch;
        bool ownVisible = searchVisible && MatchesChangeKindFilter(filter);

        bool visible = anyChildVisible || ownVisible;

        Visible(visible);

        return visible;
    }
} // namespace winrt::Flense::implementation
