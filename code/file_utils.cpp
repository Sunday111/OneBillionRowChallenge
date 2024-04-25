#include "file_utils.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>

MappedFile::MappedFile(MappedFile&& other) : data_(other.data_), fd_(other.fd_)
{
    other.data_ = {};
    other.fd_ = -1;
}

MappedFile& MappedFile::operator=(MappedFile&& other)
{
    data_ = other.data_;
    fd_ = other.fd_;
    other.data_ = {};
    other.fd_ = -1;
    return *this;
}

std::expected<MappedFile, MappedFileError> MappedFile::Open(const std::string_view file_path)
{
    auto fd = open(file_path.data(), O_RDONLY);  // NOLINT

    if (fd == -1) return std::unexpected{MappedFileError::CouldNotOpenFile};

    struct stat sb
    {
    };
    if (fstat(fd, &sb) == -1) return std::unexpected{MappedFileError::FailedToGetFileSize};

    const auto num_bytes = static_cast<size_t>(sb.st_size);
    const char* raw_data =
        reinterpret_cast<const char*>(mmap(NULL, num_bytes, PROT_WRITE, MAP_PRIVATE, fd, 0));  // NOLINT
    if (!raw_data) return std::unexpected{MappedFileError::FailedToMmap};

    return MappedFile(fd, std::string_view(raw_data, num_bytes));
}

MappedFile::~MappedFile()
{
    if (fd_ != -1)
    {
        [[maybe_unused]] const auto result = close(fd_);
        assert(result != -1);
    }
}
