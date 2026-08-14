/*
目标：观察 SIGINT 和 SIGTERM 的默认行为，不安装 signal handler。
验证：运行后用 Ctrl-C 或 kill -TERM PID，进程应被对应 signal 终止。
*/
#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "pid=" << ::getpid() << '\n'
              << "waiting for SIGINT or SIGTERM...\n"
              << std::flush;

    while (true) {
        // pause 让当前进程睡眠，直到 signal 被递送。
        // 今天使用默认处置，因此 SIGINT/SIGTERM 会直接终止进程。
        ::pause();
    }
}