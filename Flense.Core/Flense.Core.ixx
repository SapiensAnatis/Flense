export module Flense.Core;

export import :Error;
export import :Filesystem;
export import :Image;
export import :ArchiveReader;
export import :ImageParser;

// These partitions export no names, but they are imported by the interface partitions above, so they must be
// interface partitions for their definitions to be reachable by consumers. The standard requires every interface
// partition to be exported from the primary module interface.
export import :Internal.BufferPool;
export import :Internal.Channel;
export import :Internal.ChannelByteStream;
export import :Internal.Mutex;
export import :Internal.Platform;
export import :Internal.Tree;
