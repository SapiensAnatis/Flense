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

    Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> FilesystemTreeNode::Children()
    {
        if (!m_children)
        {
            std::vector<winrt::Flense::FilesystemTreeNode> children;
            children.reserve(m_node->Children().size());

            for (const auto& [name, child] : m_node->Children())
            {
                children.push_back(winrt::make<FilesystemTreeNode>(winrt::to_hstring(name), child));
            }

            m_children = winrt::single_threaded_observable_vector<winrt::Flense::FilesystemTreeNode>(std::move(children));
        }

        return m_children;
    }
} // namespace winrt::Flense::implementation
