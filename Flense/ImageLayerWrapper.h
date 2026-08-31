#pragma once

#include "ImageLayerWrapper.g.h"

import Flense.Core;

namespace winrt::Flense::implementation
{
    /// <summary>
    /// WinRT wrapper around an ImageLayer object from the core project.
    /// </summary>
    struct ImageLayerWrapper : ImageLayerWrapperT<ImageLayerWrapper>
    {
        ImageLayerWrapper(::Flense::Core::ImageLayer);

        winrt::hstring Command();
        winrt::Flense::FilesystemTreeNode FilesystemChanges();

        void UnloadTree();

      private:
        ::Flense::Core::ImageLayer m_layer;
        winrt::Flense::FilesystemTreeNode m_filesystemChanges{nullptr};
    };
} // namespace winrt::Flense::implementation
