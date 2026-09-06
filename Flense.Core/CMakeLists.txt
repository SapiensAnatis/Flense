add_library(FlenseCore STATIC)

target_compile_features(FlenseCore PUBLIC cxx_std_23)

find_package(LibArchive REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(FlenseCore PUBLIC LibArchive::LibArchive nlohmann_json::nlohmann_json)

target_sources(FlenseCore
    PRIVATE
        ChannelByteStream.cpp
        FilesystemParsing.cpp
        ImageParser.cpp
        Mutex.cpp
        Platform.Windows.cpp
    PUBLIC
    FILE_SET CXX_MODULES
    FILES
        ArchiveReader.ixx
        BufferPool.ixx
        Channel.ixx
        ChannelByteStream.ixx
        Filesystem.ixx
        FilesystemParsing.ixx
        Flense.Core.ixx
        Image.ixx
        ImageParser.ixx
        Mutex.ixx
        Platform.ixx
        Tree.ixx
)