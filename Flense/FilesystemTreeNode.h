#pragma once

#include "FilesystemTreeNode.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemTreeNode : FilesystemTreeNodeT<FilesystemTreeNode>
    {
        FilesystemTreeNode(winrt::hstring name, ::Flense::Core::FilesystemChangeTreeNodeRef node,
                           winrt::weak_ref<winrt::Flense::implementation::FilesystemTreeNode> parent);
        ~FilesystemTreeNode();

        winrt::hstring Name() const;
        winrt::Flense::FileKind Kind() const;
        winrt::Flense::FilesystemChangeKind ChangeKind() const;
        uint64_t Size() const;
        float SizeAsProportionOfParent() const;
        Microsoft::UI::Xaml::Thickness IndentMargin() const;
        bool Visible() const;
        void Visible(bool value);
        bool IsExpanded() const;
        void IsExpanded(bool value);
        bool HasChildren() const;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Children();
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> ChildrenIfExpanded();

        winrt::event_token PropertyChanged(
            const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler);
        void PropertyChanged(const winrt::event_token& token) noexcept;

        bool UpdateVisibility(std::wstring_view query, const winrt::Flense::FilesystemChangeVisibility& filter,
                              bool parentMatches);

      private:
        bool MatchesChangeKindFilter(const winrt::Flense::FilesystemChangeVisibility& filter);

        winrt::hstring m_name;
        ::Flense::Core::FilesystemChangeTreeNodeRef m_node;
        Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_children{nullptr};
        bool m_visible{true};
        bool m_isExpanded{false};
        uint32_t m_depth{0};

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
        winrt::weak_ref<winrt::Flense::implementation::FilesystemTreeNode> m_parent;
    };
} // namespace winrt::Flense::implementation
