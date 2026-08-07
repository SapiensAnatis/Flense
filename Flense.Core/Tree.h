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
        using Children = std::flat_map<std::string, Ref>;

        TreeNode(T data, Children children, Private) : m_data(std::move(data)), m_children(std::move(children))
        {
        }

        TreeNode(const TreeNode&) = delete;
        TreeNode& operator=(const TreeNode&) = delete;

        static Ref Create(T data, Children children)
        {
            return std::make_shared<const TreeNode>(std::move(data), std::move(children), Private());
        }

        [[nodiscard]] const T& Data() const
        {
            return m_data;
        }

      private:
        T m_data;
        Children m_children;
    };

    template <typename T> using TreeNodeRef = TreeNode<T>::Ref;

} // namespace Flense::Core