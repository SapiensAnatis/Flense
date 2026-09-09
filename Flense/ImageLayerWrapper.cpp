#include "pch.h"

#include "ImageLayerWrapper.h"
#if __has_include("ImageLayerWrapper.g.cpp")
#include "ImageLayerWrapper.g.cpp"
#endif

#include "FilesystemTreeNode.h"

namespace winrt::Flense::implementation
{
    ImageLayerWrapper::ImageLayerWrapper(::Flense::Core::ImageLayer layer)
        : Command(winrt::to_hstring(layer.Command())), m_layer(std::move(layer))
    {
    }

    winrt::Flense::FilesystemTreeNode ImageLayerWrapper::FilesystemChanges()
    {
        if (!m_filesystemChanges)
        {
            m_filesystemChanges = winrt::make<FilesystemTreeNode>(L"", m_layer.FilesystemChanges(), nullptr);
        }

        return m_filesystemChanges;
    }

    void ImageLayerWrapper::UnloadTree()
    {
        m_filesystemChanges = nullptr;
    }
} // namespace winrt::Flense::implementation
