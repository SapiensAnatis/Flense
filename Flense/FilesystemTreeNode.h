#pragma once

#include "FilesystemTree.h"
#include "FilesystemTreeNode.g.h"

#include <string_view>

namespace winrt::Flense::implementation
{
    struct FilesystemTreeNode : FilesystemTreeNodeT<FilesystemTreeNode>
    {
        FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node);

        winrt::hstring Name();
        winrt::Flense::FileKind Kind();
        winrt::Flense::FilesystemChangeKind ChangeKind();
        uint64_t Size();
        bool Visible();
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Children();

        winrt::event_token PropertyChanged(
            const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler);
        void PropertyChanged(const winrt::event_token& token) noexcept;

        /// <summary>
        /// Not exposed via IDL. Recomputes visibility for this node and its descendants against a search query (a
        /// node is visible if its name matches, or any descendant is visible), raising <see cref="PropertyChanged"/>
        /// only where a node's <see cref="Visible"/> actually flips. Returns whether this node ended up visible.
        /// </summary>
        bool UpdateVisibility(std::wstring_view query);

      private:
        winrt::hstring m_name;
        ::Flense::Core::FilesystemChangeTreeNodeRef m_node;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_children{nullptr};
        bool m_visible{true};

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
} // namespace winrt::Flense::implementation
