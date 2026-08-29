#include "pch.h"

#include "ImageLayerWrapper.h"
#if __has_include("ImageLayerWrapper.g.cpp")
#include "ImageLayerWrapper.g.cpp"
#endif

#include "FilesystemTreeNode.h"

namespace winrt::Flense::implementation
{
    ImageLayerWrapper::ImageLayerWrapper(::Flense::Core::ImageLayer layer) : m_layer(std::move(layer))
    {
    }

    winrt::hstring ImageLayerWrapper::Command()
    {
        // TODO: Cache strings
        return winrt::to_hstring(m_layer.Command());
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
