#include "../day2/unique_fd.hpp"

#include <cstdio>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

const char* file_type(mode_t mode) {
    if (S_ISREG(mode)) {
        return "regular file";
    }
    if (S_ISDIR(mode)) {
        return "directory";
    }
    if (S_ISCHR(mode)) {
        return "character device";
    }
    if (S_ISBLK(mode)) {
        return "block device";
    }
    if (S_ISFIFO(mode)) {
        return "fifo";
    }
    if (S_ISSOCK(mode)) {
        return "socket";
    }
    return "other";
}

void print_info(const char* label, const struct stat& info) {
    std::cout << label << '\n';
    std::cout << "  type: " << file_type(info.st_mode) << '\n';
    std::cout << "  permissions: "
              << std::oct << (info.st_mode & 0777) << std::dec << '\n';
    std::cout << "  size: "
              << static_cast<long long>(info.st_size) << " bytes\n";
    std::cout << "  device: "
              << static_cast<unsigned long long>(info.st_dev) << '\n';
    std::cout << "  inode: "
              << static_cast<unsigned long long>(info.st_ino) << '\n';
}

// ./stat_info <path>
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <path>\n", argv[0]);
        return 1;
    }

    struct stat path_info {};
    if (::stat(argv[1], &path_info) == -1) {
        ::perror("stat");
        return 1;
    }

    UniqueFd file_fd(::open(argv[1], O_RDONLY));
    if (!file_fd.valid()) {
        ::perror("open");
        return 1;
    }

    struct stat fd_info {};
    if (::fstat(file_fd.get(), &fd_info) == -1) {
        ::perror("fstat");
        return 1;
    }

    print_info("stat(path):", path_info);
    print_info("fstat(fd):", fd_info);

    const bool same_file =
        path_info.st_dev == fd_info.st_dev &&
        path_info.st_ino == fd_info.st_ino;

    std::cout << "same file object: "
              << (same_file ? "yes" : "no") << '\n';
    return 0;
}