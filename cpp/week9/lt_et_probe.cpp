#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <set>

bool set_nonblocking(int fd) {
    // 对 fd 指向的 open file description 设置 O_NONBLOCK 这个 file status flag
    // 这样能让这个 fd 对应的 socket 的 recv,send non-blocking
    const int old_flags = ::fcntl(fd, F_GETFL);
    if (old_flags == -1) {
        return false;
    }

    if (::fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) == -1) {
        return false;
    }

    return true;
}

bool send_all(int fd,const char* data) {
    std::size_t offset=0,length=strlen(data);
    // 目前需要发送的是 [offset,length)
    // 当前已经发送 offset 个 bytes
    while(offset<length) {
        const ssize_t sent=::send(fd,data+offset,length-offset,0);
        if(sent>0) {
            offset+=static_cast<std::size_t>(sent);
        } else if(sent==0) {
            // 报告错误
            std::cerr<<"error: sent=0!\n";
            return false;
        } else {
            if(errno==EINTR) {
                continue ;
            } else {
                ::perror("send");
                return false;
            }
        }
    }
    return true;
}

bool receiver_work(int fd,std::size_t limit_bytes_count,std::string& recv_str) {
    char buffer[1];
    std::size_t recv_count=0;
    while(recv_count<limit_bytes_count) {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            // 只有前 n 个 bytes 有效。
            recv_count+=static_cast<std::size_t>(n);
            std::cout<<"received: \n";
            for(std::size_t i=0;i<static_cast<std::size_t>(n);i++) {
                std::cout<<buffer[i];
                recv_str+=buffer[i];
            }
            std::cout<<'\n';  
        } else if (n == 0) {
            // Peer 的发送方向到达 EOF。
            std::cout<<"EOF\n";
            break ;
        } else if(errno==EINTR) {
            continue ;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 当前没有 bytes；稍后有 readiness event 时再尝试。
            std::cout<<"will try again!\n";
            break ;
        } else {
            // 其他错误。
            ::perror("receive");
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: ./lt_et_probe <lt|et>\n";
        return 1;
    }

    const bool edge_triggered = std::strcmp(argv[1], "et") == 0;

    if (!edge_triggered && std::strcmp(argv[1], "lt") != 0) {
        std::cerr << "mode must be lt or et\n";
        return 1;
    }

    // edge_triggered 为 true：ET
    // edge_triggered 为 false：LT
    int sockets[2]{};
    if (::socketpair(AF_UNIX,
                    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                    0,
                    sockets) == -1) {
        std::perror("socketpair");
        return 1;
    }
    const int sender_fd=sockets[0],receiver_fd=sockets[1];
    set_nonblocking(receiver_fd);
    const int epfd=::epoll_create1(EPOLL_CLOEXEC);
    if(epfd==-1) {
        std::perror("epoll_create1");
        return 1;
    }
    // 把 receiver_fd 注册到 epoll
    epoll_event interest{};
    interest.events = EPOLLIN;
    if (edge_triggered) {
        interest.events |= EPOLLET;
    }
    interest.data.fd = receiver_fd;
    if(::epoll_ctl(epfd, EPOLL_CTL_ADD, receiver_fd, &interest)==-1) {
        ::perror("epoll_ctl");
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    if(!send_all(sender_fd,"ABCDEFGH")) {
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    epoll_event returned_event{};
    int ready_count=::epoll_wait(epfd,&returned_event,1,-1);
    assert(ready_count>0);
    assert(returned_event.data.fd==receiver_fd);
    assert((returned_event.events&EPOLLIN)==EPOLLIN);
    std::cout<<"WAIT1: receiver readable\n";
    // recv 3 bytes
    std::string str;
    if(!receiver_work(receiver_fd,3,str)) {
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    assert(str=="ABC");
    // 调用第二次 wait
    // 需要设置超时时间，因为 ET 第二次是不会有通知的
    returned_event={};
    ready_count=::epoll_wait(epfd,&returned_event,1,100);
    std::cout<<"WAIT2: ";
    if(ready_count>0) {
        std::cout<<"ready\n";
    } else if(ready_count==0) {
        std::cout<<"timeout\n";
    } else {
        ::perror("epoll_wait");
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    // drain 完剩余 bytes
    str="";
    if(!receiver_work(receiver_fd,100,str)) {
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    assert(str=="DEFGH");
    if(!send_all(sender_fd,"IJ")) {
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    returned_event={};
    ready_count=::epoll_wait(epfd,&returned_event,1,100);
    std::cout<<"WAIT3: ";
    if(ready_count>0) {
        std::cout<<"ready\n";
        str="";
        if(!receiver_work(receiver_fd,100,str)) {
            ::close(epfd);
            ::close(sender_fd);
            ::close(receiver_fd);
            return 1;
        }
        assert(str=="IJ");
    } else if(ready_count==0) {
        std::cout<<"timeout\n";
    } else {
        ::perror("epoll_wait");
        ::close(epfd);
        ::close(sender_fd);
        ::close(receiver_fd);
        return 1;
    }
    ::close(epfd);
    ::close(sender_fd);
    ::close(receiver_fd);
    return 0;
}