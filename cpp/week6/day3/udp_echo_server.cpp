#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "/home/xgf/code/system-learning/cpp/week4/day2/unique_fd.hpp"

int main() {
    sockaddr_in local_address{};
    local_address.sin_family=AF_INET;
    local_address.sin_port=::htons(8080);
    const int conversion_result=::inet_pton(AF_INET,"127.0.0.1",&local_address.sin_addr);
    if(conversion_result!=1) {
        if(conversion_result==-1) {
            std::perror("inet_pton");
        } else {
            std::cerr << "invalid IPv4 text\n";
        }
        return 1;
    }
    
    // 创建一个 socket 对象
    const int raw_socket_fd=::socket(AF_INET,SOCK_DGRAM,0);
    if(raw_socket_fd==-1) {
        std::perror("socket");
        return 1;
    }
    UniqueFd socket_fd(raw_socket_fd);

    // bind 绑定到 socket object
    const int bind_result=::bind(socket_fd.get(),reinterpret_cast<const sockaddr*>(&local_address),sizeof(local_address));
    if(bind_result==-1) {
        std::perror("bind");
        return 1;
    }
    std::cout<<"server ready!"<<std::endl;
    
    // recv
    char buffer[1024]{};
    sockaddr_in peer_address{};
    socklen_t peer_length=sizeof(peer_address);
    const ssize_t received=::recvfrom(socket_fd.get(),buffer,sizeof(buffer),0,
    reinterpret_cast<sockaddr*>(&peer_address),&peer_length);
    if(received==-1) {
        std::perror("recvfrom");
        return 1;
    }

    // sendto
    const ssize_t sent=::sendto(socket_fd.get(),buffer,static_cast<std::size_t>(received),
    0,reinterpret_cast<const sockaddr*>(&peer_address),peer_length);
    if(sent==-1) {
        std::perror("sendto");
        return 1;
    }
    if(sent!=received) {
        std::cerr<<"unexpected UDP send length\n";
        return 1;
    }
    return 0;
}
