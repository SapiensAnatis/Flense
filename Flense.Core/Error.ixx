export module Flense.Core:Error;

import std;

export namespace Flense::Core
{
    /// <summary>
    /// Describes why an archive could not be read.
    /// </summary>
    enum class ArchiveErrorKind : std::uint8_t
    {
        Unspecified = 0,

        /// <summary>
        /// The stream could not be opened as an archive in any supported format.
        /// </summary>
        Unreadable,

        /// <summary>
        /// The archive opened, but an entry's header or body failed to parse part-way through.
        /// </summary>
        Corrupt,
    };

    /// <summary>
    /// Describes why an archive's contents did not amount to a container image.
    /// </summary>
    enum class ImageFormatErrorKind : std::uint8_t
    {
        Unspecified = 0,

        /// <summary>
        /// Nothing in the archive identifies it as a container image.
        /// </summary>
        NoManifest,

        /// <summary>
        /// The manifest names an image configuration the archive does not contain.
        /// </summary>
        MissingConfiguration,

        /// <summary>
        /// Image metadata was present but could not be parsed, or was not shaped as expected.
        /// </summary>
        MalformedMetadata,

        /// <summary>
        /// The configuration describes more layers than the archive carries blobs for.
        /// </summary>
        MissingLayers,
    };

    /// <summary>
    /// Base for the errors raised while reading an image, which can record the archive entry being read at the time.
    /// </summary>
    class ImageReadError : public std::runtime_error
    {
      public:
        /// <summary>
        /// The path, within the image archive, of the entry this concerns - if it concerns one.
        /// </summary>
        [[nodiscard]] const std::optional<std::string>& EntryPath() const
        {
            return m_entryPath;
        }

        void SetEntryPath(std::string entryPath)
        {
            m_entryPath = std::move(entryPath);
        }

      protected:
        explicit ImageReadError(const std::string& diagnostic, std::optional<std::string> entryPath = std::nullopt)
            : std::runtime_error(diagnostic), m_entryPath(std::move(entryPath))
        {
        }

      private:
        std::optional<std::string> m_entryPath;
    };

    /// <summary>
    /// Thrown when libarchive reports a failure that is something other than reaching the end of the archive.
    /// </summary>
    class ArchiveError : public ImageReadError
    {
      public:
        ArchiveError(ArchiveErrorKind kind, const std::string& diagnostic) : ImageReadError(diagnostic), m_kind(kind)
        {
        }

        [[nodiscard]] ArchiveErrorKind Kind() const
        {
            return m_kind;
        }

      private:
        ArchiveErrorKind m_kind;
    };

    /// <summary>
    /// Thrown when an archive is perfectly readable but does not describe a container image.
    /// </summary>
    class ImageFormatError : public ImageReadError
    {
      public:
        ImageFormatError(ImageFormatErrorKind kind, const std::string& diagnostic,
                         std::optional<std::string> entryPath = std::nullopt)
            : ImageReadError(diagnostic, std::move(entryPath)), m_kind(kind)
        {
        }

        [[nodiscard]] ImageFormatErrorKind Kind() const
        {
            return m_kind;
        }

      private:
        ImageFormatErrorKind m_kind;
    };

    /// <summary>
    /// Thrown when an operation stops early because its stop token was triggered. Not a failure, so it deliberately
    /// shares no base with the error types above beyond std::exception.
    /// </summary>
    class OperationCancelledError : public std::exception
    {
      public:
        [[nodiscard]] const char* what() const noexcept override
        {
            return "operation cancelled";
        }
    };

} // namespace Flense::Core
