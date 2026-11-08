export module Flense.Core.ImageParsing:Tree;

import std;

namespace Flense::Core
{
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

        TreeNode(T data, ChildrenContainer children, Private /*unused*/)
            : m_data(std::move(data)), m_children(std::move(children))
        {
        }

        ~TreeNode() = default;

        TreeNode(const TreeNode&) = delete;
        TreeNode& operator=(const TreeNode&) = delete;
        TreeNode(TreeNode&&) = delete;
        TreeNode& operator=(TreeNode&&) = delete;

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

    /// <summary>
    /// Applies a function to a node and each of its descendants, and returns a new tree. Returning a value equal to
    /// what was passed in results in no new node being created. Returning nullopt will remove the node and its subtree
    /// from the result.
    /// </summary>
    /// <returns>A modified tree, or nullopt if the root node was deleted.</returns>
    template <typename T, typename F>
        requires std::equality_comparable<T> && std::same_as<std::invoke_result_t<const F&, const T&>, std::optional<T>>
    std::optional<TreeNodeRef<T>> Visit(const TreeNodeRef<T>& node, const F& fn)
    {
        const auto& children = node->Children();

        std::vector<std::string> keys;
        std::vector<TreeNodeRef<T>> values;

        auto data = fn(node->Data());

        if (!data.has_value())
        {
            return std::nullopt;
        }

        keys.reserve(children.size());
        values.reserve(children.size());

        bool changed = !(data == node->Data()); // value comparison

        for (const auto& [key, child] : children)
        {
            auto next = Visit<T, F>(child, fn);

            if (next.has_value())
            {
                changed = changed || (next->get() != child.get()); // pointer comparison
                keys.push_back(key);
                values.push_back(std::move(*next));
            }
            else
            {
                // node pruned, need to recreate children
                changed = true;
            }
        }

        if (!changed)
        {
            return node; // entire subtree aliased, zero allocations
        }

        return TreeNode<T>::Create(std::move(*data), typename TreeNode<T>::ChildrenContainer(
                                                         std::sorted_unique, std::move(keys), std::move(values)));
    }
} // namespace Flense::Core
