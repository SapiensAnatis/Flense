#pragma once

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Flense::Core
{
    /// <summary>
    /// Represents a type of node in a filesystem tree.
    /// </summary>
    enum class FilesystemTreeNodeKind
    {
        Unassigned = 0,
        Directory = 1,
        File = 2,
    };

    /// <summary>
    /// Represents a node in a filesystem tree.
    /// </summary>
    class FilesystemTreeNode
    {
      public:
        FilesystemTreeNode(std::string name, FilesystemTreeNodeKind kind) : m_name(std::move(name)), m_kind(kind)
        {
        }

        /// <summary>
        /// Gets the name of the node - i.e. the name of the individual file or folder.
        /// </summary>
        /// <returns>The name of the node.</returns>
        [[nodiscard]] std::string_view Name() const
        {
            return m_name;
        }

        /// <summary>
        /// Gets a view over the node's children.
        /// </summary>
        /// <returns>A view over the node's children.</returns>
        /// <remarks>
        /// The returned span is over a vector's internal memory, so it is invalidated if AddChild is called after
        /// this method.
        /// The return type is deliberately vague to account for future changes in how the tree is implemented.
        /// </remarks>
        [[nodiscard]] std::ranges::forward_range auto Children() const
        {
            return m_children;
        }

        /// <summary>
        /// Gets the node kind.
        /// </summary>
        /// <returns>The node kind.</returns>
        [[nodiscard]] FilesystemTreeNodeKind Kind() const
        {
            return m_kind;
        }

        /// <summary>
        /// Adds a child to the node.
        /// </summary>
        /// <param name="name">The name of the new node.</param>
        /// <param name="kind">The kind of the new node.</param>
        /// <returns>A reference to the newly added node.</returns>
        FilesystemTreeNode& AddChild(std::string_view name, FilesystemTreeNodeKind kind)
        {
            return m_children.emplace_back(std::move(name), kind);
        }

        /// <summary>
        /// Gets an existing child based on the values of name and kind, or if it does not exist, adds a new a child
        /// with the given properties.
        /// </summary>
        /// <param name="name">The name of the node to get or add.</param>
        /// <param name="kind">The kind of the node to get or add.</param>
        /// <returns>A reference to the newly added node.</returns>
        FilesystemTreeNode& GetOrAddChild(std::string_view name, FilesystemTreeNodeKind kind)
        {
            if (auto it = std::ranges::find_if(m_children,
                                               [name, kind](const FilesystemTreeNode& node) {
                                                   return node.Name() == name && node.Kind() == kind;
                                               });
                it != m_children.end())
            {
                return *it;
            }

            return AddChild(name, kind);
        }

      private:
        std::string m_name;
        FilesystemTreeNodeKind m_kind;
        std::vector<FilesystemTreeNode> m_children;
    };

    class FilesystemTree
    {
      public:
        explicit FilesystemTree(FilesystemTreeNode root) : m_root(std::move(root))
        {
        }

        /// <summary>
        /// Gets a reference to the root element.
        /// </summary>
        /// <returns>A reference to the root element.</returns>
        [[nodiscard]] FilesystemTreeNode& Root()
        {
            return m_root;
        }

      private:
        FilesystemTreeNode m_root;
    };
} // namespace Flense::Core