/*
功能：
1. 打开用户指定的目标文件。
2. 使用 dup2 把标准输出 fd 1 重定向到该文件。
3. 分别通过 std::cout、printf 和 write 输出，验证三者都会进入文件。
4. 保留 stderr 的原去向，用它输出诊断信息和可选的 lsof 观察提示。

用法：
    ./redirect_stdout <output-file>
    ./redirect_stdout <output-file> --pause

第二种用法会暂停等待回车，便于在另一个终端用 lsof 查看 fd 0、1、2。
*/
#include "../day2/unique_fd.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <string_view>
#include <unistd.h>

// 把 data 中的 size 个字节全部写入 fd。
// 成功返回 true；失败时打印 errno 对应信息并返回 false。
// fd 只是借用，本函数不拥有它，也不会 close(fd)。
bool write_all(int fd, const char* data, std::size_t size) {
    std::size_t written = 0;

    while (written < size) {
        const ssize_t count =
            ::write(fd, data + written, size - written);

        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }
            ::perror("write");
            return false;
        }

        // 防止极少见的“没有报错但也没有前进”导致死循环。
        if (count == 0) {
            std::fprintf(stderr, "write returned 0 before completion\n");
            return false;
        }

        written += static_cast<std::size_t>(count);
    }

    return true;
}

// 从标准输入 fd 0 读取一个字符，用于让进程暂停，方便运行 lsof。
// 成功或遇到 EOF 返回 true；真实读取错误返回 false。
// STDIN_FILENO 只是借用，本函数不负责关闭它。
bool wait_for_input() {
    char ignored = '\0';

    while (true) {
        const ssize_t count = ::read(STDIN_FILENO, &ignored, 1);
        if (count >= 0) {
            return true;
        }
        if (errno == EINTR) {
            continue;
        }

        ::perror("read stdin");
        return false;
    }
}

// ./redirect_stdout redirected.txt
// 程序入口：解析目标路径，完成标准输出重定向，并验证三个输出接口的去向。
int main(int argc, char* argv[]) {
    const bool pause_for_lsof =
        argc == 3 && std::string_view(argv[2]) == "--pause";

    if (argc != 2 && !pause_for_lsof) {
        std::fprintf(
            stderr,
            "usage: %s <output-file> [--pause]\n",
            argv[0]);
        return 1;
    }

    // dup2 将要改变 fd 1。先 flush，确保这行仍然进入当前终端。
    std::cout << "before redirect: this line goes to the terminal\n"
              << std::flush;

    // output_fd 是 owning fd，由 UniqueFd 管理。
    UniqueFd output_fd(
        ::open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644));
    if (!output_fd.valid()) {
        ::perror("open output");
        return 1;
    }

    // 复制方向：output_fd 是来源，fd 1 是要被改写的目标表项。
    if (::dup2(output_fd.get(), STDOUT_FILENO) == -1) {
        ::perror("dup2 stdout");
        return 1;
    }

    // 正常启动时 fd 1 原本已被占用，所以 open 通常返回 3 或更大。
    // dup2 成功后，fd 1 已独立保留对文件的引用，可以关闭多余的原 fd。
    // 如果程序启动时 fd 1 本来就是关闭的，open 可能直接返回 1；此时不能提前关闭它。
    if (output_fd.get() != STDOUT_FILENO) {
        output_fd = UniqueFd{};
    }

    // C++ 标准输出：现在最终进入 fd 1 指向的文件。
    std::cout << "written by std::cout\n" << std::flush;
    if (!std::cout) {
        std::fprintf(stderr, "std::cout write failed\n");
        return 1;
    }

    // C 标准输出 stdout：显式 fflush，避免内容继续停留在用户态缓冲区。
    if (std::printf("written by printf\n") < 0 ||
        std::fflush(stdout) == EOF) {
        ::perror("printf/fflush");
        return 1;
    }

    // 直接向标准输出 fd 1 写入，不经过 C/C++ 格式化流。
    constexpr char raw_message[] = "written by write\n";
    if (!write_all(
            STDOUT_FILENO,
            raw_message,
            sizeof(raw_message) - 1)) {
        return 1;
    }

    // fd 2 没有被 dup2 修改，因此这条诊断通常仍显示在终端。
    std::fprintf(
        stderr,
        "stderr: stdout now points to %s\n",
        argv[1]);

    if (pause_for_lsof) {
        std::fprintf(
            stderr,
            "pid=%ld, inspect it from another terminal, then press Enter\n",
            static_cast<long>(::getpid()));

        if (!wait_for_input()) {
            return 1;
        }
    }

    return 0;
}