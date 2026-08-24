#include "pch.h"

#include "FilesystemTreeNode.h"
#if __has_include("FilesystemTreeNode.g.cpp")
#include "FilesystemTreeNode.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    FilesystemTreeNode::FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node)
        : m_name(std::move(name)), m_node(std::move(node))
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

    Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemTreeNode::
        Children()
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

    winrt::event_token FilesystemTreeNode::PropertyChanged(
        const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void FilesystemTreeNode::PropertyChanged(const winrt::event_token& token) noexcept
    {
        m_propertyChanged.remove(token);
    }

    bool FilesystemTreeNode::UpdateVisibility(std::wstring_view query)
    {
        bool anyChildVisible = false;

        for (const auto& child : Children())
        {
            anyChildVisible |= winrt::get_self<FilesystemTreeNode>(child)->UpdateVisibility(query);
        }

        bool visible = anyChildVisible || query.empty() || std::wstring_view{m_name}.contains(query);

        if (m_visible != visible)
        {
            m_visible = visible;
            m_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs{L"Visible"});
        }

        return visible;
    }
} // namespace winrt::Flense::implementation
