#pragma once

#include <expected>
#include <string_view>

enum class MappedFileError
{
    CouldNotOpenFile,
    FailedToGetFileSize,
    FailedToMmap
};

class MappedFile
{
public:
    static std::expected<MappedFile, MappedFileError> Open(const std::string_view path);
    MappedFile(const MappedFile&) = delete;
    MappedFile(MappedFile&&);
    MappedFile& operator=(const MappedFile&) = delete;
    MappedFile& operator=(MappedFile&&);
    ~MappedFile();

    std::string_view GetData() const
    {
        return data_;
    }

private:
    MappedFile(const int fd, std::string_view data) : data_(data), fd_(fd) {}

private:
    std::string_view data_;
    int fd_ = -1;
};
