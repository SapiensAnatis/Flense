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
        bool IsExpanded();
        void IsExpanded(bool value);
        bool HasChildren();
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Children();

        winrt::event_token PropertyChanged(
            const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler);
        void PropertyChanged(const winrt::event_token& token) noexcept;

        bool UpdateVisibility(std::wstring_view query, bool parentMatches);

      private:
        winrt::hstring m_name;
        ::Flense::Core::FilesystemChangeTreeNodeRef m_node;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_children{nullptr};
        bool m_visible{true};
        bool m_isExpanded{false};

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
} // namespace winrt::Flense::implementation
