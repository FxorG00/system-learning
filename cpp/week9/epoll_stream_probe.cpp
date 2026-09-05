#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>

bool set_nonblocking(int fd) {
    const int old_flags = ::fcntl(fd, F_GETFL);
    if (old_flags == -1) {
        return false;
    }

    if (::fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) == -1) {
        return false;
    }

    return true;
}

const char payload[]="this is a test!";
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

bool sender_work(int fd) {
    // 向自己的 fd 对应的 socket 发送信息，最后会去到 peer socket 的 receive queue
    // 因为这个实验创建的是一对已经建立链接的 socketpair
    const char* data=payload;
    return send_all(fd,data);
}

std::vector<char>recv_data;
bool receiver_work(int fd) {
    char buffer[2];
    while(1) {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            // 只有前 n 个 bytes 有效。
            std::cout<<"received: \n";
            for(std::size_t i=0;i<static_cast<std::size_t>(n);i++) {
                std::cout<<buffer[i];
                recv_data.push_back(buffer[i]);
            }
            std::cout<<'\n';
        } else if (n == 0) {
            // Peer 的发送方向到达 EOF。
            std::cout<<"EOF\n";
            return false;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 当前没有 bytes；稍后有 readiness event 时再尝试。
            std::cout<<"will try again!\n";
            return true;
        } else {
            // 其他错误。
            ::perror("receive");
            return false;
        }
    }
    return true;
}

bool check() {
    if(recv_data.size()!=strlen(payload)) {
        return false;
    }
    for(std::size_t i=0;i<strlen(payload);i++) {
        if(payload[i]!=recv_data[i]) {
            return false;
        }
    }
    return true;
}

int main() {
    int sockets[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1) {
        return 1;
    }
    const int sender_fd=sockets[0],receiver_fd=sockets[1];
    if(!set_nonblocking(receiver_fd)) {
        ::close(receiver_fd);
        ::close(sender_fd);
        return 1;
    }
    // 使用一个 epoll instance 注册 receiver_fd
    const int epfd=::epoll_create1(EPOLL_CLOEXEC);
    if(epfd==-1) {
        std::perror("epoll_create1");
        ::close(receiver_fd);
        ::close(sender_fd);
        return 1;
    }
    epoll_event interest{};
    interest.events=EPOLLIN;
    interest.data.fd=receiver_fd;
    if(::epoll_ctl(epfd,EPOLL_CTL_ADD,receiver_fd,&interest)==-1) {
        std::perror("epoll_ctl ADD");
        ::close(epfd);
        ::close(receiver_fd);
        ::close(sender_fd);
        return 1;
    }
    epoll_event returned_event{};
    int ready_count=::epoll_wait(epfd,&returned_event,1,0);
    // A. before data：immediate wait returns 0
    assert(ready_count==0);
    // B. after send：wait returns an event for receiver with EPOLLIN
    if(!sender_work(sender_fd)) {
        ::close(epfd);
        ::close(receiver_fd);
        ::close(sender_fd);
        return 1;
    }
    ready_count=::epoll_wait(epfd,&returned_event,1,100);
    assert(ready_count==1);
    assert((returned_event.events&EPOLLIN)!=0U);
    // 还需要检查 returned_event.data.fd=receiver_fd
    assert(returned_event.data.fd==receiver_fd);
    std::cout<<"data.fd="<<returned_event.data.fd<<'\n';
    // C. before consume：default LT reports receiver again
    // 就是再次 epoll_wait，此时仍然是 readable readiness
    ready_count=::epoll_wait(epfd,&returned_event,1,100);
    assert(ready_count==1);
    assert((returned_event.events&EPOLLIN)!=0U);
    // 还需要检查 returned_event.data.fd=receiver_fd
    assert(returned_event.data.fd==receiver_fd);
    // D. drain：reconstructed payload exact match, 
    // final recv state is EAGAIN/EWOULDBLOCK
    // 就是一直 recv 直至 EAGAIN，把 queue 里的信息读完
    if(!receiver_work(receiver_fd)) {
        ::close(epfd);
        ::close(receiver_fd);
        ::close(sender_fd);
        return 1;
    }
    // E. after drain：immediate wait returns 0
    ready_count=::epoll_wait(epfd,&returned_event,1,100);
    assert(ready_count==0);
    assert(check());
    std::cout<<"PASS\n";
    ::close(epfd);
    ::close(receiver_fd);
    ::close(sender_fd);
    return 0;
}