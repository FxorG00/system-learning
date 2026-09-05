// Observe one accept4 on an empty non-blocking listener; no client is needed.
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>

int main() {
    const int listener = ::socket(
        AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listener == -1) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(0);  // Kernel chooses an available local port.
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(listener, reinterpret_cast<const sockaddr*>(&address),
               sizeof(address)) == -1) {
        std::perror("bind");
        ::close(listener);
        return 1;
    }
    if (::listen(listener, 8) == -1) {
        std::perror("listen");
        ::close(listener);
        return 1;
    }

    const int connection = ::accept4(
        listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
    const int saved_errno = errno;
    const bool empty = connection == -1 &&
        (saved_errno == EAGAIN || saved_errno == EWOULDBLOCK);
    if (connection >= 0) {
        ::close(connection);
    } else if (!empty) {
        errno = saved_errno;
        std::perror("accept4");
    }
    const int close_result = ::close(listener);
    if (close_result == -1) {
        std::perror("close listener");
        return 1;
    }
    if (!empty) {
        std::fprintf(stderr, "expected an empty accept queue\n");
        return 1;
    }
    std::puts("PASS: empty accept queue -> would block");
    return 0;
}