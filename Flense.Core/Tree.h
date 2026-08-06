#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

        TreeNode(T data, std::vector<Ref> children, Private) : m_data(std::move(data)), m_children(std::move(children))
        {
        }

        TreeNode(const TreeNode&) = delete;
        TreeNode& operator=(const TreeNode&) = delete;

        static Ref Create(T data, std::vector<Ref> children = {})
        {
            return std::make_shared<const TreeNode>(std::move(data), std::move(children), Private());
        }

        [[nodiscard]] const T& Data() const
        {
            return m_data;
        }

        [[nodiscard]] std::span<const Ref> Children() const
        {
            return std::span(m_children);
        }

      private:
        T m_data;
        std::vector<Ref> m_children;
    };

    template <typename T> using TreeNodeRef = TreeNode<T>::Ref;

} // namespace Flense::Core