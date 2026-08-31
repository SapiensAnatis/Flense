export module Flense.Core:ImageLayer;

import :FilesystemTree;
import std;

export namespace Flense::Core
{
    class ImageLayer
    {
      public:
        ImageLayer(std::string command, FilesystemChangeTreeNodeRef filesystemChanges)
            : m_command(std::move(command)), m_filesystemChanges(std::move(filesystemChanges))
        {
        }

        [[nodiscard]] std::string_view Command() const
        {
            return m_command;
        }

        [[nodiscard]] FilesystemChangeTreeNodeRef FilesystemChanges() const
        {
            return m_filesystemChanges;
        }

      private:
        std::string m_command;
        FilesystemChangeTreeNodeRef m_filesystemChanges;
    };
} // namespace Flense::Core
