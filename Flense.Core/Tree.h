#pragma once

#include <algorithm>
#include <concepts>
#include <flat_map>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace Flense::Core
{
    template <typename T> class TreeNode;

    /// <summary>
    /// Represents an immutable tree or tree node.
    /// </summary>
    template <typename T> class TreeNode : public std::enable_shared_from_this<const TreeNode<T>>
    {
        struct Private
        {
            explicit Private() = default;
        };

      public:
        using Ref = std::shared_ptr<const TreeNode>;
        using ChildrenContainer = std::flat_map<std::string, Ref>;

        TreeNode(T data, ChildrenContainer children, Private) : m_data(std::move(data)), m_children(std::move(children))
        {
        }

        TreeNode(const TreeNode&) = delete;
        TreeNode& operator=(const TreeNode&) = delete;

        static Ref Create(T data, ChildrenContainer children)
        {
            return std::make_shared<const TreeNode>(std::move(data), std::move(children), Private());
        }

        [[nodiscard]] const T& Data() const
        {
            return m_data;
        }

        [[nodiscard]] const ChildrenContainer& Children() const
        {
            return m_children;
        }

      private:
        T m_data;
        ChildrenContainer m_children;
    };

    // Don't use TreeNode<T>::Ref; that would hinder template parameter deducation for Visit
    template <typename T> using TreeNodeRef = std::shared_ptr<const TreeNode<T>>;

    template <typename T, typename F>
        requires std::equality_comparable<T> && std::same_as<std::invoke_result_t<F&, const T&>, T>
    TreeNodeRef<T> Visit(const TreeNodeRef<T>& node, F&& fn)
    {
        const auto& children = node->Children();

        std::vector<TreeNodeRef<T>> values;
        values.reserve(children.size());

        bool changed = false;

        for (const auto& child : children.values())
        {
            auto next = Visit(child, fn);
            changed = changed || (next.get() != child.get()); // pointer comparison
            values.push_back(std::move(next));
        }

        auto data = fn(node->Data());
        changed = changed || !(data == node->Data()); // value comparison

        if (!changed)
        {
            return node; // entire subtree aliased, zero allocations
        }

        return TreeNode<T>::Create(
            std::move(data), TreeNode<T>::ChildrenContainer(std::sorted_unique, children.keys(), std::move(values)));
    }

    template <typename T, typename F, typename U = std::invoke_result_t<F&, const T&>>
        requires(!std::same_as<U, T>)
    TreeNodeRef<T> Visit(const TreeNodeRef<T>& node, F&& fn)
    {
        const auto& children = node->Children();

        std::vector<TreeNodeRef<U>> values;
        values.reserve(children.size());

        for (const auto& child : children.values())
        {
            auto next = Visit(child, fn);
            values.push_back(std::move(next));
        }

        auto data = fn(node->Data());

        return TreeNode<U>::Create(
            std::move(data), TreeNode<U>::ChildrenContainer(std::sorted_unique, children.keys(), std::move(values)));
    }

} // namespace Flense::Core