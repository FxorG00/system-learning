#include <cerrno>  // errno 及相关错误码；perror 会解释当前 errno
#include <cstddef> // std::size_t
#include <cstdio>  // fprintf、perror、stderr
#include <fcntl.h> // open、O_RDONLY
#include <unistd.h> // read、write、close、STDOUT_FILENO

// write 可能只写出一部分数据，所以循环，直到 size 字节全部写完。
bool write_all(int fd, const char* data, std::size_t size) {
    std::size_t total_written = 0;

    while (total_written < size) {
        // ::write 使用全局命名空间中的 POSIX write，不是 std 里的函数。
        // 从尚未写出的第一个字节开始，尝试写出剩余部分。
        const ssize_t written = ::write(
            fd,
            data + total_written,
            size - total_written);

        if (written == -1) {
            // write 失败时会设置 errno。
            // perror 会向标准错误 stderr 输出："write: errno 对应的说明"。
            ::perror("write");
            return false;
        }

        if (written == 0) {
            // fprintf 的第一个参数决定写到哪个 C 流。
            // stderr 是标准错误流，通常对应文件描述符 2。
            // 这里防止 write 一直返回 0，导致循环无法前进。
            ::fprintf(stderr, "write returned 0 before completion\n");
            return false;
        }

        // 进入这里时 written > 0，因此可以安全地转换为无符号的 size_t。
        total_written += static_cast<std::size_t>(written);
    }

    return true;
}

// argc：argument count 命令行参数数量，包含程序名本身。
// argv：argument vector 参数字符串数组。
// 运行 ./mycat test.txt 时：argc == 2，argv[0] == "./mycat"，argv[1] == "test.txt"。
int main(int argc, char* argv[]) {
    if (argc != 2) {
        // fprintf(stderr, ...) 表示把格式化文字写到标准错误，而不是正常输出。
        // %s 会被 argv[0] 替换，所以可能输出：usage: ./mycat <file>
        ::fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return 1;
    }

    // argv[1] 是用户提供的文件路径。
    // O_RDONLY 表示 read only：只读打开，不创建文件，也不允许通过该 fd 写文件。
    // open 成功返回非负 fd；失败返回 -1，并设置 errno。
    const int fd = ::open(argv[1], O_RDONLY);
    if (fd == -1) {
        // 例如文件不存在时可能输出：open: No such file or directory
        ::perror("open");
        return 1;
    }

    // read 把原始字节写进这个缓冲区，不会自动在末尾添加 '\0'。
    char buffer[4096];
    bool success = true;

    while (true) {
        // 最多读取 sizeof(buffer) 字节。
        // count > 0：本次实际读取的字节数。
        // count == 0：到达 EOF。
        // count == -1：读取失败，并设置 errno。
        const ssize_t count = ::read(fd, buffer, sizeof(buffer));

        if (count > 0) {
            // STDOUT_FILENO 通常是 fd 1，也就是标准输出。
            // 只输出本次真正读到的 count 字节，不能把整个 buffer 当 C 字符串。
            if (!write_all(
                    STDOUT_FILENO,
                    buffer,
                    static_cast<std::size_t>(count))) {
                success = false;
                break;
            }
            // 本次读取成功，继续下一次 read，直到遇到 EOF。
            continue;
        }

        if (count == 0) {
            // EOF 不是错误，说明文件已经正常读完。
            break;
        }

        // 能走到这里说明 count == -1。
        ::perror("read");
        success = false;
        break;
    }

    // 只要 open 成功，这个文件 fd 就由当前程序负责关闭。
    // 即使 read/write 失败，也会离开循环并走到这里。
    if (::close(fd) == -1) {
        ::perror("close");
        success = false;
    }

    // Unix 约定：0 表示成功，非 0 表示失败。
    return success ? 0 : 1;
}