#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <cstring>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <set>
#include <map>
#define ADD_FLAG 0
#define DEL_FLAG 1
void clear_connection(int epfd,int fd,std::set<int>& fds);

bool alive(int fd,std::set<int>& fds) {
    return fds.find(fd)!=fds.end();
}

class ConnectionState {
public:
    ConnectionState(){}
    ConnectionState(int FD,uint32_t status):fd(FD),registration_status(status) {}
    // 追加 str 这部分 bytes
    // return true 意味着 output 此时不为空
    bool append_char(const char& ch) {
        input.push_back(ch);
        if(ch==delimiter) {
            // 这是分隔符
            for(auto c:input) {
                output.push_back(c);
            }
            ++message_count_;
            input.clear();
            return true;
        }
        return false;
    }

    const std::string& pending_input() const {
        return input;
    }
    const std::string& pending_output() const {
        return output;
    }
    bool output_empty() const {
        return output.empty();
    }
    std::size_t message_count() const {
        return message_count_;
    }
    // return false 发送失败
    // 需要外层去关闭 connection
    bool send_output(std::set<int>& fds) {
        if(!alive(fd,fds)) {
            // fd 已经失效
            return false;
        }
        // offset 是 output 已经发送出去的 bytes
        // 当前需要发送 [offset,output.size())
        std::size_t this_time_sent=0;
        while(offset<output.size()) {
            const ssize_t n=::send(fd,output.c_str()+offset,output.size()-offset,MSG_NOSIGNAL);
            if(n>0) {
                // 发送成功
                offset+=static_cast<std::size_t>(n);
                this_time_sent+=static_cast<std::size_t>(n);
            } else if(n<0) {
                if(errno==EINTR) {
                    continue ;
                } else if(errno==EAGAIN||errno==EWOULDBLOCK) {
                    // 当前没办法再立即发送了，本次 send 结束
                    std::cout<<"fd="<<fd<<" "<<"send "<<this_time_sent<<" bytes\n";
                    std::cout<<"remaining="<<output.size()-offset<<" bytes\n";
                    return true;
                } else if(errno==EPIPE) {
                    // 目前 connection 无法再发送了
                    return false;
                } else {
                    // 其他错误
                    return false;
                }
            } else {
                // n==0 视为错误
                return false;
            }
        }
        // 清空一下 output
        output.clear();
        offset=0;
        // std::cout<<"fd="<<fd<<" "<<"send "<<this_time_sent<<" bytes\n";
        // std::cout<<"remaining=0 bytes\n";
        return true;
    }
    void set_peer_write_closed(bool value) {
        peer_write_closed=value;
    }
    bool get_peer_write_closed() const {
        return peer_write_closed;
    }
    uint32_t get_registration_status() const {
        return registration_status;
    }
    void set_registration_status(uint32_t status) {
        registration_status=status;
    }
private:
    // 直接默认构造即可
    std::string input="",output="";
    std::size_t message_count_=0;
    static const char delimiter='\n';
    int fd=0; // 属于哪个 fd
    std::size_t offset=0;
    bool peer_write_closed=false;
    uint32_t registration_status=0;
};
std::map<int,ConnectionState>connection_state;
bool edge_triggered;
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

void clear_connection(int epfd,int fd,std::set<int>& fds) {
    if(!alive(fd,fds)) {
        // fd 已经失效
        return ;
    }
    std::cout<<"CLOSE fd="<<fd<<'\n';
    const int result = ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    ::close(fd);
    fds.erase(fds.find(fd));
    if(result<0) {
        std::perror("epoll_ctl DEL");
    }
    connection_state.erase(fd);
}

// return false; 意味着遇到了错误
bool update_epoll_status(int epfd,int fd,std::set<int>& fds,uint32_t bitmask,bool add_or_del) {
    // add_or_del=0: add
    if(!alive(fd,fds)) {
        return false;
    }
    uint32_t old_status=connection_state[fd].get_registration_status();
    epoll_event interest{};
    interest.events = old_status;
    if(add_or_del==ADD_FLAG) {
        // add
        interest.events|=bitmask;
    } else {
        // del
        if(interest.events&bitmask) {
            interest.events^=bitmask;
        }
    }
    interest.data.fd = fd;
    connection_state[fd].set_registration_status(interest.events);
    if (::epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&interest) == -1) {
        std::perror("epoll_ctl MOD");
        return false;
    }
    return true;
}

void receiver_work(int epfd,int fd,std::set<int>& fds) {
    if(!alive(fd,fds)) {
        return ;
    }
    char buffer[1024];
    std::size_t this_time_recv=0;
    // std::cout<<"recv\n";
    while(1) {
        const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n > 0) {
            // 只有前 n 个 bytes 有效。
            this_time_recv+=n;
            for(std::size_t i=0;i<static_cast<std::size_t>(n);i++) {
                if(connection_state[fd].append_char(buffer[i])) {
                    if(!update_epoll_status(epfd,fd,fds,EPOLLOUT,ADD_FLAG)) {
                        clear_connection(epfd,fd,fds);
                        return ;
                    }
                }
            }
            // std::cout<<"this time recv "<<n<<" bytes \n";
        } else if (n == 0) {
            // Peer 的发送方向到达 EOF。
            // 可以设置 peer_write_closed
            connection_state[fd].set_peer_write_closed(true);
            std::cout<<"fd="<<fd<<" recv "<<this_time_recv<<" bytes\n";
            std::cout<<"EOF\n";
            return ;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 当前没有 bytes；稍后有 readiness event 时再尝试。
            std::cout<<"fd="<<fd<<" recv "<<this_time_recv<<" bytes\n";
            std::cout<<"will try again!\n";
            return ;
        } else if(errno!=EINTR) {
            // 其他错误。
            // 这个 connection 发生了错误！
            // 也需要主动去 close 这个 fd，并且去 DEL 以及 erase
            ::perror("receive");
            clear_connection(epfd,fd,fds);
            return ;
        }
    }
}

void sender_work(int epfd,int fd,std::set<int>& fds) {
    if(!alive(fd,fds)) {
        return ;
    }
    // std::cout<<"send work\n";
    if(!connection_state[fd].send_output(fds)) {
        // 调用 send_output 中 失败，需要清理 connection
        clear_connection(epfd,fd,fds);
        return ;
    }
    if(connection_state[fd].output_empty()) {
        if(!update_epoll_status(epfd,fd,fds,EPOLLOUT,DEL_FLAG)) {
            clear_connection(epfd,fd,fds);
        }
    }
}

bool init(int& listener) {
    listener = ::socket(
        AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (listener == -1) {
        std::perror("socket");
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(9091);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(listener, reinterpret_cast<const sockaddr*>(&address),
               sizeof(address)) == -1) {
        std::perror("bind");
        ::close(listener);
        return false;
    }
    if (::listen(listener, 8) == -1) {
        std::perror("listen");
        ::close(listener);
        return false;
    }

    char ip[INET_ADDRSTRLEN]{};
    if (::inet_ntop(AF_INET, &address.sin_addr, ip, sizeof(ip)) == nullptr) {
        std::perror("inet_ntop");
        return false;
    }
    const std::uint16_t port = ntohs(address.sin_port);
    std::cout<<"LISTEN \n";
    std::cout << "IP   = " << ip << '\n';
    std::cout << "port = " << port << '\n';
    return true;
}

bool register_to_epoll(int epfd,int fd,uint32_t bitmask) {
    epoll_event interest{};
    interest.events=bitmask|(edge_triggered?EPOLLET:0U)|EPOLLRDHUP;
    interest.data.fd=fd;
    if(::epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&interest)==-1) {
        std::perror("epoll_ctl ADD");
        ::close(fd);
        return false;
    }
    return true;
}

void listener_handler(int listener,int epfd,std::set<int>& fds) {
    // 立即尝试 accept 得到新的 connection
    while(1) {
        const int connection = ::accept4(listener, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        std::cout<<connection<<'\n';
        if(connection>=0) {
            std::cout<<"ACCEPT fd="<<connection<<'\n';
            if(!register_to_epoll(epfd,connection,EPOLLIN)) {
                std::perror("epoll_ctl ADD");
                ::close(connection);
                continue ;
            }
            // 初始化
            connection_state[connection]=ConnectionState(connection,EPOLLIN|(edge_triggered?EPOLLET:0U)|EPOLLRDHUP);
            fds.insert(connection);
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
    std::cout<<"bye\n";
}

void connection_handler(int fd,int epfd,std::set<int>& fds,const epoll_event& returned_event) {
    // 是 fd
    // 那我需要去判断时 EPOLLIN/OUT
    // std::cout<<"hi\n";
    if(!alive(fd,fds)) {
        return ;
    }
    // std::cout<<"hi\n";
    if((returned_event.events&EPOLLERR)==EPOLLERR) {
        int socket_error = 0;
        socklen_t length = sizeof(socket_error);
        if (::getsockopt(fd,
                        SOL_SOCKET,
                        SO_ERROR,
                        &socket_error,
                        &length) == -1) {
            std::perror("getsockopt SO_ERROR");
        } else if (socket_error != 0) {
            std::cerr << "socket error: " << std::strerror(socket_error) << '\n';
        }
        clear_connection(epfd,fd,fds);
        return ;
    }
    if((returned_event.events&EPOLLRDHUP)==EPOLLRDHUP) {
        receiver_work(epfd,fd,fds);
        if(alive(fd,fds)) {
            connection_state[fd].set_peer_write_closed(true);
            // 目前 peer_write_closed 了，那么我把我 output 给发出去其实连接就可以关闭了
            sender_work(epfd,fd,fds);
        }
    }
    if((returned_event.events&EPOLLHUP)==EPOLLHUP) {
        receiver_work(epfd,fd,fds);
        if(alive(fd,fds)) {
            connection_state[fd].set_peer_write_closed(true);
            // 目前 peer_write_closed 了，那么我把我 output 给发出去其实连接就可以关闭了
            sender_work(epfd,fd,fds);
        }
    }
    if((returned_event.events&EPOLLIN)==EPOLLIN) {
        receiver_work(epfd,fd,fds);
    }
    if((returned_event.events&EPOLLOUT)==EPOLLOUT) {
        sender_work(epfd,fd,fds);
    }
    if(alive(fd,fds)&&connection_state[fd].get_peer_write_closed()) {
        // 刚刚我已经 drain 到 EAGAIN/EOF 了
        // 并且对面不可能再发信息过来了
        // 意味着我可以不用注册 EPOLLIN 了
        update_epoll_status(epfd,fd,fds,EPOLLIN,DEL_FLAG);
        if(connection_state[fd].output_empty()) {
            clear_connection(epfd,fd,fds);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: ./epoll_echo_server <lt|et>\n";
        return 1;
    }
    edge_triggered = std::strcmp(argv[1], "et") == 0;
    if (!edge_triggered && std::strcmp(argv[1], "lt") != 0) {
        std::cerr << "mode must be lt or et\n";
        return 1;
    }

    int listener;
    if(!init(listener)) {
        return 1;
    }
    // 创建 epoll，关注 listener,connection_fd
    const int epfd=::epoll_create1(EPOLL_CLOEXEC);
    if(epfd==-1) {
        std::perror("epoll_create1");
        ::close(listener);
        return 1;
    }
    // 向 epoll 注册 listener
    if(!register_to_epoll(epfd,listener,EPOLLIN)) {
        return 1;
    }
    std::set<int> fds;
    // 用 set 管理 fds
    // fds 存放尚未 close 的 fd
    fds.insert(listener);
    while(1) {
        // 每次都去 epoll_wait，看看能不能去对某个 fd 做 recv
        // 设置超时时间为 -1 能真正睡到事件发生1
        epoll_event returned_event{};
        int ready_count=::epoll_wait(epfd,&returned_event,1,-1);
        // std::cout<<"ready"
        // std::cout<<ready_count<<'\n';
        if(ready_count>0) {
            // 有 ready 的
            int fd=returned_event.data.fd;
            // 去看是不是 listener 
            if(fd==listener) {
                listener_handler(fd,epfd,fds);
            } else {
                connection_handler(fd,epfd,fds,returned_event);
            }
        } else if(ready_count<0) {
            if(errno!=EINTR) {
                std::perror("epoll_wait");
                for(auto fd:fds) {
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