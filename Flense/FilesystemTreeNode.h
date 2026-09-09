#pragma once

#include "FilesystemTreeNode.g.h"
#include "winrt/Flense.h"

#include <string_view>

import Flense.Core;

namespace winrt::Flense::implementation
{
    struct FilesystemTreeNode : FilesystemTreeNodeT<FilesystemTreeNode>,
                                wil::notify_property_changed_base<FilesystemTreeNode>
    {
        FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node,
                           winrt::weak_ref<winrt::Flense::FilesystemTreeNode> parent);
        ~FilesystemTreeNode();

        wil::single_threaded_property<winrt::hstring> Name;

        winrt::Flense::FileKind Kind() const;
        winrt::Flense::FilesystemChangeKind ChangeKind() const;
        uint64_t Size() const;
        float SizeAsProportionOfParent() const;
        Microsoft::UI::Xaml::Thickness IndentMargin() const;

        // Settable internally only; not exposed as a setter via IDL
        wil::single_threaded_notifying_property<bool> Visible;

        bool IsExpanded() const;
        void IsExpanded(bool value);
        bool HasChildren() const;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Children();
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> ChildrenIfExpanded();

        bool UpdateVisibility(std::wstring_view query, const winrt::Flense::FilesystemChangeVisibility& filter,
                              bool parentMatches);

      private:
        bool MatchesChangeKindFilter(const winrt::Flense::FilesystemChangeVisibility& filter);

        ::Flense::Core::FilesystemChangeTreeNodeRef m_node;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_children{nullptr};
        bool m_isExpanded{false};
        uint32_t m_depth{0};

        winrt::weak_ref<winrt::Flense::FilesystemTreeNode> m_parent;
    };
} // namespace winrt::Flense::implementation
