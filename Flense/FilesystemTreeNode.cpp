#include "pch.h"

#include "FilesystemTreeNode.h"
#if __has_include("FilesystemTreeNode.g.cpp")
#include "FilesystemTreeNode.g.cpp"
#endif

using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Microsoft::UI::Xaml::Data;
using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::Flense::implementation
{
    FilesystemTreeNode::FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node,
                                           winrt::weak_ref<winrt::Flense::FilesystemTreeNode> parent)
        : m_name(std::move(name)), m_node(std::move(node)), m_parent(std::move(parent))
    {
        if (auto parentProjected = m_parent.get())
        {
            m_depth = winrt::get_self<FilesystemTreeNode>(parentProjected)->m_depth + 1;
        }
    }

    FilesystemTreeNode::~FilesystemTreeNode() = default;

    winrt::hstring FilesystemTreeNode::Name() const
    {
        return m_name;
    }

    winrt::Flense::FileKind FilesystemTreeNode::Kind() const
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

    winrt::Flense::FilesystemChangeKind FilesystemTreeNode::ChangeKind() const
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

    uint64_t FilesystemTreeNode::Size() const
    {
        return m_node->Data().size;
    }

    float FilesystemTreeNode::SizeAsProportionOfParent() const
    {
        // This isn't implemented in the Flense::Core tree because it is bad for node reusability

        if (auto parent = m_parent.get(); parent && parent.Size() != 0)
        {
            return static_cast<float>(m_node->Data().size) / static_cast<float>(parent.Size());
        }
        else
        {
            return 0;
        }
    }

    Thickness FilesystemTreeNode::IndentMargin() const
    {
        static constexpr double IndentSizeInPixels = 12.0;
        return {.Left = m_depth * IndentSizeInPixels, .Top = 0, .Right = 0, .Bottom = 0};
    }

    bool FilesystemTreeNode::Visible() const
    {
        return m_visible;
    }

    /// <remarks>Not exposed via IDL, intended for internal use only</remarks>
    void FilesystemTreeNode::Visible(bool value)
    {
        if (m_visible != value)
        {
            m_visible = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"Visible"});
        }
    }

    bool FilesystemTreeNode::IsExpanded() const
    {
        return m_isExpanded;
    }

    void FilesystemTreeNode::IsExpanded(bool value)
    {
        if (m_isExpanded != value)
        {
            m_isExpanded = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"IsExpanded"});
            m_propertyChanged(*this, PropertyChangedEventArgs{L"ChildrenIfExpanded"});
        }
    }

    IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemTreeNode::Children()
    {
        if (!m_children)
        {
            const winrt::Flense::FilesystemTreeNode self = *this;

            std::vector<winrt::Flense::FilesystemTreeNode> children;
            children.reserve(m_node->Children().size());

            for (const auto& [name, child] : m_node->Children())
            {
                children.push_back(
                    winrt::make<FilesystemTreeNode>(winrt::to_hstring(name), child, winrt::make_weak(self)));
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

    bool FilesystemTreeNode::HasChildren() const
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
    bool FilesystemTreeNode::UpdateVisibility(std::wstring_view query,
                                              const winrt::Flense::FilesystemChangeVisibility& filter,
                                              bool parentMatches)
    {
        // TODO: This causes the entire tree to be materialized by calling Children() on every node, we could find a
        // way to improve this, e.g. searching over the core tree type and materializing only matching nodes.
        bool anyChildVisible = false;

        const bool thisNodeMatch = std::wstring_view{m_name}.contains(query);

        for (const auto& child : Children())
        {
            anyChildVisible |= winrt::get_self<FilesystemTreeNode>(child)->UpdateVisibility(
                query, filter, parentMatches || thisNodeMatch);
        }

        const bool searchVisible = parentMatches || query.empty() || thisNodeMatch;
        const bool ownVisible = searchVisible && MatchesChangeKindFilter(filter);

        const bool visible = anyChildVisible || ownVisible;

        Visible(visible);

        return visible;
    }
} // namespace winrt::Flense::implementation
