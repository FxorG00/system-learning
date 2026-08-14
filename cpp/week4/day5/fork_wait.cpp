// 创建一个子进程
// 父子通过 fork 返回值进入不同分支
// 子进程用退出码 7 结束
// 父进程用 waitpid 等待指定子进程
// 父进程正确解析子进程的终止方式和退出码
#include <cerrno>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// 等待指定子进程。
// 输入：child_pid 是要等待的子进程 PID；status 指向接收终止状态的 int。
// 输出：成功返回被回收子进程的 PID，失败返回 -1。
// waitpid 被信号打断并设置 errno=EINTR 时重试，其他错误交给调用者处理。
pid_t waitpid_retry(pid_t child_pid, int* status) {
    while (true) {
        const pid_t result = ::waitpid(child_pid, status, 0);
        if (result == -1 && errno == EINTR) {
            continue;
        }
        return result;
    }
}

int main() {
    // fork 会复制当前用户态缓冲区的状态；先刷新，避免这行被父子重复刷新。
    std::cout << "before fork: pid=" << ::getpid() << '\n' << std::flush;

    const pid_t child_pid = ::fork();
    if (child_pid == -1) {
        // fork 失败时没有子进程，errno 记录失败原因。
        std::perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        // 只有子进程进入这里。子进程看到 fork 的返回值为 0，
        // 但它自己的真实 PID 要通过 getpid() 获取。
        std::cout << "child: pid=" << ::getpid()
                  << ", ppid=" << ::getppid()
                  << ", fork returned 0\n";
        return 7;
    }

    // 只有父进程进入这里。父进程拿到的是新子进程的 PID。
    std::cout << "parent: pid=" << ::getpid()
              << ", child_pid=" << child_pid
              << ", waiting...\n";

    int status = 0;
    const pid_t waited_pid = waitpid_retry(child_pid, &status);
    if (waited_pid == -1) {
        std::perror("waitpid");
        return 1;
    }

    // status 是编码后的等待状态，必须先判断终止类型，再取对应字段。
    if (WIFEXITED(status)) {
        std::cout << "parent: child " << waited_pid
                  << " exited normally, exit status="
                  << WEXITSTATUS(status) << '\n';
    } else if (WIFSIGNALED(status)) {
        std::cout << "parent: child " << waited_pid
                  << " was terminated by signal "
                  << WTERMSIG(status) << '\n';
    } else {
        std::cout << "parent: child changed state in another way\n";
    }

    return 0;
}