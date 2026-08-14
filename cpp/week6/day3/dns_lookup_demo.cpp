#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

/*
目标：
    让 libc resolver 把 example.com 解析为适合 IPv4 UDP socket 的 address。
验证：
    程序打印 getaddrinfo 返回列表中的第一个 IPv4 address。
*/
int main() {
    addrinfo hints{};
    hints.ai_family = AF_INET;       // 只观察 IPv4。
    hints.ai_socktype = SOCK_DGRAM;  // 只要适合 UDP socket 的 address。

    addrinfo* result = nullptr;
    const int error =
        ::getaddrinfo("baidu.com", "53", &hints, &result);

    if (error != 0) {
        std::cerr << "getaddrinfo: "
                  << ::gai_strerror(error)
                  << '\n';
        return 1;
    }

    // result 可能有多个 nodes；这里只观察第一个 IPv4 address。
    const auto* ipv4_address =
        reinterpret_cast<const sockaddr_in*>(result->ai_addr);

    char address_text[INET_ADDRSTRLEN]{};
    if (::inet_ntop(AF_INET,
                    &ipv4_address->sin_addr,
                    address_text,
                    sizeof(address_text)) == nullptr) {
        std::perror("inet_ntop");
        ::freeaddrinfo(result);
        return 1;
    }

    std::cout << address_text << '\n';
    ::freeaddrinfo(result);
    return 0;
}