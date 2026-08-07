#pragma once

#include "FilesystemTree.h"
#include "FilesystemTreeNode.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemTreeNode : FilesystemTreeNodeT<FilesystemTreeNode>
    {
        FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node);

        winrt::hstring Name();
        bool IsDirectory();
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Children();

      private:
        winrt::hstring m_name;
        ::Flense::Core::FilesystemChangeTreeNodeRef m_node;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_children{nullptr};
    };
} // namespace winrt::Flense::implementation
