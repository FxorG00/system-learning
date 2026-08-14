#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "../../week4/day2/unique_fd.hpp"
#include <cstdint>

bool send_all(int fd,const char* data,std::size_t size) {
    // 当前未发送 [offset,size)
    // offset 就是已经发送的 bytes 数量
    // 未发送 bytes 数量 size-offset
    std::size_t offset=0;
    while(offset<size) {
        const ssize_t sent = ::send(
            fd,
            data+offset,
            size-offset,
            MSG_NOSIGNAL
        );
        if (sent == -1) {
            if(errno==EINTR) {
                continue ;
            }
            std::perror("send");
            return false;
        } else if(sent==0) {
            std::cerr<<"sent 0 bytes\n";
            return false;
        }
        offset+=static_cast<std::size_t>(sent);
    }
    return true;
}

void work_one_connection(int listening_fd) {
    // while(1) accept 到 connected_fd
    // 避免因为 EINTR 打断
    int raw_connected_fd;
    sockaddr_in peer_address{};
    while(1) {
        socklen_t peer_length = sizeof(peer_address);
        raw_connected_fd = ::accept(
            listening_fd,
            reinterpret_cast<sockaddr*>(&peer_address),
            &peer_length
        );
        if (raw_connected_fd == -1) {
            if(errno==EINTR) {
                continue ;
            }
            std::perror("accept");
            return ;
        } else {
            break ;
        }
    }
    UniqueFd connected_fd(raw_connected_fd);
    const std::uint16_t network_port = ::ntohs(peer_address.sin_port);
    std::cout<<"peer: "<<network_port<<"\n";
    char inet_ntop_text[INET_ADDRSTRLEN]{};
    const char* inet_ntop_result =::inet_ntop(AF_INET, &peer_address.sin_addr, inet_ntop_text, sizeof(inet_ntop_text));
    if(inet_ntop_result==nullptr) {
        std::perror("inet_ntop");
        return ;
    }
    std::cout<<inet_ntop_result<<'\n';

    // recv
    // 循环读到 recv=0 peer EOF
    char buffer[1024]{};
    while(1) {
        const ssize_t received = ::recv(
            connected_fd.get(),
            buffer,
            sizeof(buffer),
            0
        );
        if (received == -1) {
            if(errno==EINTR) {
                continue ;
            }
            std::perror("recv");
            break ;
        } else if (received == 0) {
            std::cout << "peer closed its sending side\n";
            break ;
        } else {
            std::cout<<"recv\n";
            for(std::size_t i=0;i<static_cast<std::size_t>(received);i++) {
                std::cout<<buffer[i];
            }
            if(!send_all(connected_fd.get(),buffer,static_cast<std::size_t>(received))) {
                break ;
            }
        }
    }
    std::cout<<"over over!\n";
}

int main() {
    // 创建一个 socket obje ct
    // 此时啥也没干
    const int raw_listening_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (raw_listening_fd == -1) {
        std::perror("socket");
        return 1;
    }

    UniqueFd listening_fd(raw_listening_fd);

    // setsockopt 
    const int enable = 1;
    if (::setsockopt(
            listening_fd.get(),
            SOL_SOCKET,
            SO_REUSEADDR,
            &enable,
            sizeof(enable)) == -1) {
        std::perror("setsockopt");
        return 1;
    }
    // 设置 local_address
    sockaddr_in local_address{};
    local_address.sin_family=AF_INET;
    local_address.sin_port=::htons(18080);
    const int conversion_result=::inet_pton(AF_INET,"127.0.0.1",&local_address.sin_addr);
    if(conversion_result!=1) {
        if(conversion_result==-1) {
            std::perror("inet_pton");
        } else {
            std::cerr << "invalid IPv4 text\n";
        }
        return 1;
    }
    // bind 绑定到 socket object
    const int bind_result=::bind(listening_fd.get(),reinterpret_cast<const sockaddr*>(&local_address),sizeof(local_address));
    if(bind_result==-1) {
        std::perror("bind");
        return 1;
    }
    // listen 让 socket 变成 listening socket
    constexpr int kBacklog = 8;
    if (::listen(listening_fd.get(), kBacklog) == -1) {
        std::perror("listen");
        return 1;
    }
    std::cout<<"server ready!"<<std::endl;
    // accept 取出一个 pending connection
    // 得到的 fd 指向 connected socket
    while(1) {
        work_one_connection(listening_fd.get());
    }
    return 0;
}