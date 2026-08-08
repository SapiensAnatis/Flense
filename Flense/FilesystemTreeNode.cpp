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

    bool FilesystemTreeNode::IsDirectory()
    {
        return m_node->Data().kind == ::Flense::Core::FileKind::Directory;
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
} // namespace winrt::Flense::implementation
