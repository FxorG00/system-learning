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

bool recv_exact(int fd,char* buffer,std::size_t expected) {
    // 从 server 接收 expected 个 bytes，存到 buffer 里面
    // 已经接收 offset 个 bytes
    // 剩余 [offset,expected) 这一段
    std::size_t offset=0;
    char recv_buffer[1024]{};
    while(offset<expected) {
        const ssize_t received = ::recv(fd,recv_buffer,expected-offset,0);
        if(received>0) {
            const std::size_t len=static_cast<std::size_t>(received);
            for(std::size_t i=0;i<len;i++) {
                buffer[offset+i]=recv_buffer[i];
            }
            offset+=len;
        } else if(received==0) {
            std::cerr<<"not recv exeac expected bytes\n";
            return false;
        } else {
            if(errno==EINTR) {
                continue ;
            } else {
                std::perror("recv");
                return false;
            }
        }
    }
    return true;
}

bool recv_the_rest(int fd) {
    // 继续 recv 知道 recv==0
    // 虽然我 shutdown WR
    // 但是 server 还是可以向 client 发送信息
    // 接收到 EOF 的时候退出
    char buffer[1024]{};
    while(1) {
        const ssize_t received=::recv(fd,buffer,sizeof(buffer),0);
        if(received>0) {
            const std::size_t len=static_cast<std::size_t>(received);
            for(std::size_t i=0;i<len;i++) {
                std::cout<<buffer[i];
            }
        } else if(received==0) {
            // std::cout<<"recv server EOF\n";
            return true;
        } else {
            if(errno==EINTR) {
                continue ;
            } else {
                std::perror("recv");
                return false;
            }
        }
    }
    return true;
}

int main() {
    // 创建一个 socket
    const int raw_socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (raw_socket_fd == -1) {
        std::perror("socket");
        return 1;
    }
    UniqueFd socket_fd(raw_socket_fd);
    // std::cout<<"ha";

    // 设置 server address
    sockaddr_in server_address{};
    server_address.sin_family=AF_INET;
    server_address.sin_port=::htons(18080);
    const int conversion_result=::inet_pton(AF_INET,"127.0.0.1",&server_address.sin_addr);
    if(conversion_result!=1) {
        if(conversion_result==-1) {
            std::perror("inet_pton");
        } else {
            std::cerr << "invalid IPv4 text\n";
        }
        return 1;
    }
    // connect 
    if (::connect(
        socket_fd.get(),
        reinterpret_cast<const sockaddr*>(&server_address),
        sizeof(server_address)) == -1) {
        std::perror("connect");
        return 1;
    }
    // std::cout<<"connected!\n";
    const int buffer_capacity=1024;
    char buffer[buffer_capacity];
    while(1) {
        const ssize_t count=::read(STDIN_FILENO,buffer,sizeof(buffer));
        // 读一些 bytes，就发给 server 这些 bytes
        if(count>0) {
            // 表示 buffer[0,count-1] 是有效的
            const std::size_t len=static_cast<std::size_t>(count);
            if(!send_all(socket_fd.get(),buffer,len)) {
                std::cerr<<"send_all error!\n";
                return 1;
            }
            
            // send all 了，等待 server echo 回来
            // 用 recv_exact 接收到 buffer 里面
            if(!recv_exact(socket_fd.get(),buffer,len)) {
                return 1;
            }
            // std::cout<<"ok\n";
            for(std::size_t i=0;i<len;i++) {
                std::cout<<buffer[i];
            }
        } else if(count==0) {
            // EOF
            break ;
        } else {
            if(errno==EINTR) {
                continue ;
            } else {
                ::perror("read");
                return 1;
            }
        }
    }
    if (::shutdown(socket_fd.get(), SHUT_WR) == -1) {
        std::perror("shutdown");
        return 1;
    }
    if(!recv_the_rest(socket_fd.get())) {
        return 1;
    }
    return 0;
}