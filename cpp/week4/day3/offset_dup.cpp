/*
功能：验证 dup 得到的 fd 会与原 fd 共享文件偏移，
     而重新 open 同一路径得到的 fd 拥有独立的文件偏移。

运行过程：
1. original_fd 打开文件。
2. duplicated_fd 由 dup(original_fd) 得到，与 original_fd 共享打开状态。
3. independent_fd 重新 open 同一路径，拥有另一份独立的打开状态。
4. 交替读取并修改偏移，通过输出观察三者的关系。
*/
#include "../day2/unique_fd.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

// 从 fd 最多读取 size 字节到 buffer。
// 如果 read 被信号中断并返回 EINTR，就重新尝试；其他结果直接交给调用者处理。
// fd 只是借用，本函数不拥有它，也不会 close(fd)。
ssize_t read_retry(int fd, char* buffer, std::size_t size) {
    while (true) {
        const ssize_t count = ::read(fd, buffer, size);
        if (count == -1 && errno == EINTR) {
            continue;
        }
        return count;
    }
}

// 从 fd 读取 3 字节，然后报告“读取内容”和“读取后的当前偏移”。
// label 只用于区分输出来自哪个 fd；成功返回 true，失败返回 false。
// 与 read_retry 一样，本函数只借用 fd，不负责关闭它。
bool read_and_report(const char* label, int fd) {
    char buffer[3];

    // read 成功后，会推进 fd 对应 open file description 中的 offset。
    const ssize_t count = read_retry(fd, buffer, sizeof(buffer));

    if (count == -1) {
        ::perror("read");
        return false;
    }

    // SEEK_CUR 表示以当前位置为基准；移动量为 0，因此只查询、不改变当前 offset。
    const off_t current = ::lseek(fd, 0, SEEK_CUR);
    if (current == static_cast<off_t>(-1)) {
        ::perror("lseek current");
        return false;
    }

    std::cout << label << " fd=" << fd << " read=\"";
    // buffer 不是以 '\0' 结尾的 C 字符串，所以按 count 指定的实际字节数输出。
    std::cout.write(buffer, count);
    std::cout << "\" offset="
              << static_cast<long long>(current) << '\n';
    return true;
}

// 程序入口：要求命令行提供一个文件路径，并完成“dup 与重新 open”的对照实验。
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    // 第一次 open：创建一份 open file description，初始 offset 为 0。
    // UniqueFd 拥有返回的 fd，并在离开作用域时自动 close。
    UniqueFd original_fd(::open(argv[1], O_RDONLY));
    if (!original_fd.valid()) {
        ::perror("open original");
        return 1;
    }

    // dup 创建一个新的 fd 表入口，但它与 original_fd 指向同一个
    // open file description，因此二者共享 offset 和文件状态 flags。
    UniqueFd duplicated_fd(::dup(original_fd.get()));
    if (!duplicated_fd.valid()) {
        ::perror("dup");
        return 1;
    }

    // 第二次 open 同一路径：创建另一份 open file description。
    // 它指向同一个文件对象，但拥有独立的 offset，初始值同样为 0。
    UniqueFd independent_fd(::open(argv[1], O_RDONLY));
    if (!independent_fd.valid()) {
        ::perror("open independent");
        return 1;
    }

    // original 先读 3 字节，把共享 offset 从 0 推进到 3。
    if (!read_and_report("original   ", original_fd.get())) {
        return 1;
    }
    // duplicated 从共享 offset 3 开始读，而不是从文件开头读。
    if (!read_and_report("duplicated ", duplicated_fd.get())) {
        return 1;
    }
    // independent 有自己的 offset，所以第一次读取仍从 0 开始。
    if (!read_and_report("independent", independent_fd.get())) {
        return 1;
    }

    // 通过 duplicated_fd 把共享 offset 设置为距离文件开头 1 字节的位置。
    // 因为 original_fd 共享同一份打开状态，它下一次读取也会从位置 1 开始。
    const off_t new_offset =
        ::lseek(duplicated_fd.get(), 1, SEEK_SET);
    if (new_offset == static_cast<off_t>(-1)) {
        ::perror("lseek set");
        return 1;
    }

    std::cout << "lseek duplicated to offset "
              << static_cast<long long>(new_offset) << '\n';

    // 预期从位置 1 读出 BCD，证明 duplicated 对 offset 的修改影响了 original。
    if (!read_and_report("original   ", original_fd.get())) {
        return 1;
    }
    // independent 的 offset 未被 lseek 影响，继续从自己的位置 3 读出 DEF。
    if (!read_and_report("independent", independent_fd.get())) {
        return 1;
    }

    // main 返回后，三个 UniqueFd 分别关闭三个不同的 fd 表入口。
    return 0;
}