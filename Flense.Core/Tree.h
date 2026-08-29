#pragma once

import std;

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

    /// <summary>
    /// Prunes nodes and their children from a tree based on a predicate.
    /// </summary>
    /// <typeparam name="T">The type of tree element.</typeparam>
    /// <typeparam name="F">The type of predicate function.</typeparam>
    /// <param name="node">The node to start at.</param>
    /// <param name="predicate">The predicate function. If it returns true, the node and its children will not be
    /// present in the returned tree.</param>
    /// <returns>A new tree with selected nodes removed.</returns>
    /// <remarks>The predicate is not called on the root node. So if a function that always returns true is passed, then
    /// the result will be a tree with a childless root node.</remarks>
    template <typename T, typename F>
        requires std::same_as<std::invoke_result_t<F&, const T&>, bool>
    TreeNodeRef<T> Prune(const TreeNodeRef<T>& node, F&& predicate)
    {
        // Do not call predicate() on the root node...
        const auto& children = node->Children();

        std::vector<TreeNodeRef<T>> values;
        values.reserve(children.size());

        std::vector<std::string> keys;
        keys.reserve(children.size());

        bool changed = false;

        for (const auto& [key, child] : children)
        {
            if (!predicate(child->Data()))
            {
                auto next = Prune(child, predicate);
                changed = changed || (next.get() != child.get()); // pointer comparison
                values.push_back(std::move(next));
                keys.push_back(key);
            }
            else
            {
                changed = true;
            }
        }

        if (!changed)
        {
            return node; // entire subtree aliased, zero allocations
        }

        return TreeNode<T>::Create(
            node->Data(), TreeNode<T>::ChildrenContainer(std::sorted_unique, std::move(keys), std::move(values)));
    }

} // namespace Flense::Core