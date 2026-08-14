/*
目标：
    把一段 IPv4 text 转换为 in_addr 中的 network binary form。
验证：
    程序根据 inet_pton 的三类返回值打印结果。
*/
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
void work_host_port() {
    const std::uint16_t host_port = 8080;

    const std::uint16_t network_port = ::htons(host_port);
    const std::uint16_t restored_port = ::ntohs(network_port);

    std::cout << "host port: " << std::dec<<host_port << '\n';
    std::cout<<"network bytes: ";
    const auto* bytes=reinterpret_cast<const unsigned char*>(&network_port);
    for(std::size_t i=0;i<sizeof(network_port);i++) {
        std::cout<<std::hex<<std::setw(2)<<static_cast<unsigned int>(bytes[i])<<' ';
    }
    std::cout<<'\n';
    std::cout<<"port round trip: "<<std::dec<<restored_port<<'\n';
}

void test_wrong() {
    const char* text = "192.168.56.888";
    in_addr address{};  // output object：转换成功后，binary IPv4 写到这里
    // std::cout<<"IPv4 input: "<<text<<'\n';
    const int result = ::inet_pton(AF_INET, text, &address);
    if (result == 0) {
        std::cerr << "invalid IPv4 text\n";
    }
}

int main() {
    const char* text = "192.168.56.129";
    in_addr address{};  // output object：转换成功后，binary IPv4 写到这里
    std::cout<<"IPv4 input: "<<text<<'\n';
    const int result = ::inet_pton(AF_INET, text, &address);
    if (result == 0) {
        std::cerr << "invalid IPv4 text\n";
        return 1;
    } else if (result == 1) {
        // std::cout<<address.s_addr<<'\n';
        // 8 bit = 1 Byte
        std::cout<<"IPv4 bytes: ";
        const auto* bytes=reinterpret_cast<const unsigned char*>(&address.s_addr);
        for(std::size_t i=0;i<sizeof(address.s_addr);i++) {
            std::cout<<std::hex<<std::setw(2)<<static_cast<unsigned int>(bytes[i])<<' ';
        }
        std::cout<<std::endl;
    } else {
        std::perror("inet_pton");
        return 1;
    }
    char inet_ntop_text[INET_ADDRSTRLEN]{};
    const char* inet_ntop_result =::inet_ntop(AF_INET, &address, inet_ntop_text, sizeof(inet_ntop_text));
    if(inet_ntop_result==nullptr) {
        std::perror("inet_ntop");
        return 1;
    }
    std::cout<<"IPv4 round trip: "<<inet_ntop_result<<'\n';
    work_host_port();
    test_wrong();
    return 0;
}