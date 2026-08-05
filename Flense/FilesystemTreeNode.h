#pragma once

#include "FilesystemTreeNode.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemTreeNode : FilesystemTreeNodeT<FilesystemTreeNode>
    {
        FilesystemTreeNode() = default;

        winrt::hstring Name();
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Children();

      private:
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_children;
    };
} // namespace winrt::Flense::implementation
