#pragma once

#include <string>
#include <string_view>

namespace Flense::Core
{

    class ImageLayer
    {
      public:
        ImageLayer(std::string command) : m_command(std::move(command))
        {
        }

        std::string_view Command() const
        {
            return m_command;
        }

      private:
        std::string m_command;
    };

} // namespace Flense::Core