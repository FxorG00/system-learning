#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// 等待指定子进程结束。
// 输入：child_pid 是目标子进程；没有输出参数，因为本实验不读取退出码。
// 输出：成功返回 true；除 EINTR 外的错误返回 false，并保留 errno 供 perror 使用。
bool wait_for_child(pid_t child_pid) {
    while (true) {
        const pid_t result = ::waitpid(child_pid, nullptr, 0);
        if (result == child_pid) {
            return true;
        }
        if (result == -1 && errno == EINTR) {
            continue;
        }
        return false;
    }
}

int main() {
    int value = 10;

    std::cout << "before fork: pid=" << ::getpid()
              << ", value=" << value
              << ", address=" << static_cast<const void*>(&value)
              << '\n' << std::flush;

    const pid_t child_pid = ::fork();
    if (child_pid == -1) {
        std::perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        value = 99;
        std::cout << "child: pid=" << ::getpid()
                  << ", value=" << value
                  << ", address=" << static_cast<const void*>(&value)
                  << '\n' << std::flush;

        // std::flush 已把这次输出送出用户态缓冲区。
        // _exit 直接终止子进程，不执行普通的 C/C++ 用户态退出清理。
        ::_exit(0);
    }

    if (!wait_for_child(child_pid)) {
        std::perror("waitpid");
        return 1;
    }

    std::cout << "parent after wait: pid=" << ::getpid()
              << ", value=" << value
              << ", address=" << static_cast<const void*>(&value)
              << '\n';

    return 0;
}