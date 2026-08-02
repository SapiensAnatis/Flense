#include "pch.h"

#include "ArchiveReader.h"

#include <archive.h>
#include <archive_entry.h>

namespace Flense::Core
{
    ArchiveReader::ArchiveReader() : m_archive{archive_read_new()}
    {
        archive_read_support_format_tar(m_archive.get());
    }

    void ArchiveReader::ProcessTarBytes(std::span<const std::byte> bytes)
    {
    }
} // namespace Flense::Core