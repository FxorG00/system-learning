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
    const int old_flags = ::fcntl(fd, F_GETFL);
    if (old_flags == -1) {
        return false;
    }

    if (::fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) == -1) {
        return false;
    }

    return true;
}

void receiver_work(int connection_epoll_fd,int fd,std::set<int>& connection_fds) {
    // std::cout<<"now recv fd="<<fd<<'\n';
    char buffer[1024];
    while(1) {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            // 只有前 n 个 bytes 有效。
            std::cout<<"DATA fd="<<fd<<" n="<<n<<": ";
            for(std::size_t i=0;i<static_cast<std::size_t>(n);i++) {
                std::cout<<buffer[i];
            }
            std::cout<<'\n';
        } else if (n == 0) {
            // Peer 的发送方向到达 EOF。
            // 意味着 peer 已经关闭了
            // 那么我已经读完了，则这个 socket 对应的 queue 后面也不会再有消息了，可以关闭了
            std::cout<<"EOF\n";
            std::cout<<"CLOSE fd="<<fd<<'\n';
            const int result = ::epoll_ctl(connection_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
            ::close(fd);
            connection_fds.erase(connection_fds.find(fd));
            if(result<0) {
                std::perror("epoll_ctl DEL");
            }
            return ;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 当前没有 bytes；稍后有 readiness event 时再尝试。
            std::cout<<"will try again!\n";
            return ;
        } else if(errno!=EINTR) {
            // 其他错误。
            // 这个 connection 发生了错误！
            // 也需要主动去 close 这个 fd，并且去 DEL 以及 erase
            ::perror("receive");
            std::cout<<"CLOSE fd="<<fd<<'\n';
            const int result = ::epoll_ctl(connection_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
            ::close(fd);
            connection_fds.erase(connection_fds.find(fd));
            if(result<0) {
                std::perror("epoll_ctl DEL");
            }
            return ;
        }
    }
}

int main() {
    const int listener = ::socket(
        AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listener == -1) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(9090);
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

    char ip[INET_ADDRSTRLEN]{};
    if (::inet_ntop(AF_INET, &address.sin_addr, ip, sizeof(ip)) == nullptr) {
        std::perror("inet_ntop");
        return 1;
    }
    const std::uint16_t port = ntohs(address.sin_port);
    std::cout<<"LISTEN \n";
    std::cout << "IP   = " << ip << '\n';
    std::cout << "port = " << port << '\n';

    // 创建 epoll，关注 listener,connection_fd
    const int epfd=::epoll_create1(EPOLL_CLOEXEC);
    if(epfd==-1) {
        std::perror("epoll_create1");
        ::close(listener);
        return 1;
    }
    // 向 epoll 注册 listener
    epoll_event interest{};
    interest.events=EPOLLIN;
    interest.data.fd=listener;
    if(::epoll_ctl(epfd,EPOLL_CTL_ADD,listener,&interest)==-1) {
        std::perror("epoll_ctl ADD");
        ::close(epfd);
        ::close(listener);
        return 1;
    }
    std::set<int> connection_fds;
    // 用 set 管理 fds
    while(1) {
        // 每次都去 epoll_wait，看看能不能去对某个 fd 做 recv
        // 设置超时时间为 -1 能真正睡到事件发生1
        epoll_event returned_event{};
        int ready_count=::epoll_wait(epfd,&returned_event,1,-1);
        if(ready_count>0) {
            // 有 ready 的
            int fd=returned_event.data.fd;
            // 去看是不是 listener 
            if(fd==listener) {
                // 是的话说明可以立即尝试 accept 得到新的 connection
                while(1) {
                    const int connection = ::accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
                    if(connection>=0) {
                        std::cout<<"ACCEPT fd="<<connection<<'\n';
                        epoll_event interest{};
                        interest.events=EPOLLIN;
                        interest.data.fd=connection;
                        if(::epoll_ctl(epfd,EPOLL_CTL_ADD,connection,&interest)==-1) {
                            std::perror("epoll_ctl ADD");
                            ::close(connection);
                        } else {
                            connection_fds.insert(connection);
                        }
                    } else if(errno==EINTR) {
                        continue ;
                    } else if(errno==EAGAIN||errno==EWOULDBLOCK) {
                        // 成功榨干，当前没有立即能拿到的 pending connection 了
                        // 结束本次 accept loop
                        break ;
                    } else {
                        std::perror("accept4");
                        break ;
                    }
                }
            } else {
                // 是 connection_fd
                receiver_work(epfd,fd,connection_fds);
            }
        } else if(ready_count<0) {
            if(errno!=EINTR) {
                std::perror("epoll_wait");
                for(auto fd:connection_fds) {
                    ::close(fd);
                }
                ::close(listener);
                ::close(epfd);
                return 1;
            }
        }
    }
    return 0;
}