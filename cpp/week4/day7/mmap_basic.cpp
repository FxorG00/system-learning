/*
目标：把一个小型普通文件只读映射到当前进程的虚拟地址空间，
      关闭原 fd 后通过 mapping 输出文件内容，最后解除映射。
验证：非空文件输出应与原内容一致；空文件正常退出；不存在文件报告错误。
*/
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// ./mmap_basic note.txt
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }
    // std::cout<<argv[0]<<std::endl;
    const int fd = ::open(argv[1], O_RDONLY);
    if (fd == -1) {
        ::perror("open");
        return 1;
    }

    struct stat file_info {};
    if (::fstat(fd, &file_info) == -1) {
        ::perror("fstat");
        ::close(fd);
        return 1;
    }

    // mmap 的 length 必须大于 0；空文件没有字节需要映射。
    if (file_info.st_size == 0) {
        if (::close(fd) == -1) {
            ::perror("close");
            return 1;
        }
        return 0;
    }

    if (file_info.st_size < 0) {
        std::fprintf(stderr, "invalid negative file size\n");
        ::close(fd);
        return 1;
    }

    const std::size_t length =
        static_cast<std::size_t>(file_info.st_size);

    // 创建当前进程的只读私有文件映射，让内核选择起始地址。
    void* const address = ::mmap(
        nullptr,
        length,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0
    );

    if (address == MAP_FAILED) {
        ::perror("mmap");
        ::close(fd);
        return 1;
    }

    // mmap 成功后，关闭 fd 不会使已经建立的 mapping 失效。
    if (::close(fd) == -1) {
        ::perror("close");
        ::munmap(address, length);
        return 1;
    }

    const auto* const bytes = static_cast<const char*>(address);

    // mapping 不是 C 字符串，只能访问 [0, length)，不能寻找 '\0'。
    for (std::size_t i = 0; i < length; ++i) {
        std::cout.put(bytes[i]);
    }

    if (!std::cout) {
        std::fprintf(stderr, "stdout write failed\n");
        ::munmap(address, length);
        return 1;
    }

    // 解除成功后，bytes/address 都不能再用于访问这段区域。
    if (::munmap(address, length) == -1) {
        ::perror("munmap");
        return 1;
    }

    return 0;
}