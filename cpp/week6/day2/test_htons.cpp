/*
目标：
    把 host-side port 转成 network representation，再转换回来。
验证：
    round trip 后仍然得到原来的 port 8080。
*/
#include <arpa/inet.h>
#include <cstdint>
#include <iostream>

int main() {
    const std::uint16_t host_port = 8080;

    const std::uint16_t network_port = ::htons(host_port);
    const std::uint16_t restored_port = ::ntohs(network_port);

    std::cout << "host port: " << host_port << '\n';
    std::cout<<"network port: "<<network_port<<'\n';
    std::cout << "restored port: " << restored_port << '\n';
    return 0;
}