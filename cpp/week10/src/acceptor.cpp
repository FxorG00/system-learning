#include "acceptor.hpp"
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <arpa/inet.h>
#include <iostream>
void system_error_helper(int error_code,std::string msg) {
    throw std::system_error(error_code,std::generic_category(),msg);
}

// channel 是不支持任何 copy/move assignment/constructor
// 所以需要直接在初始化列表中构造
Acceptor::Acceptor(EventLoop& loop, std::uint16_t port, int backlog):
loop_(loop),listener_(::socket(
        AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),listener_channel_(listener_.get()),backlog_(backlog) {
    // 任何 setup 失败
    // 抛 system_error，并且 close 已经创建成功的 fd
    if(backlog<=0) {
        throw std::invalid_argument("backlog<=0");
    }
    if (!listener_) {
        const int error_code=errno;
        system_error_helper(error_code,"socket constructor");
    }
    // setsockopt SO_REUSEADDR  
    const int enabled = 1;
    if (::setsockopt(listener_.get(), SOL_SOCKET, SO_REUSEADDR,
                    &enabled, sizeof(enabled)) == -1) {
        throw std::system_error(errno, std::generic_category(), "setsockopt");
    }
    // bind
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(listener_.get(), reinterpret_cast<const sockaddr*>(&address),
               sizeof(address)) == -1) {
        const int error_code=errno;
        system_error_helper(error_code,"bind");
    }
    // 最后再存 port_，转换成 host byte order
    // 并且要去取得 socket object 绑定的端口号
    sockaddr_in bound_address{};
    socklen_t length = sizeof(bound_address);
    if (::getsockname(
            listener_.get(),
            reinterpret_cast<sockaddr*>(&bound_address),
            &length) == -1) {
        const int error_code=errno;
        system_error_helper(error_code,"getsockname");
    }

    port_ = ::ntohs(bound_address.sin_port);
}
Acceptor::~Acceptor() {
    if(start_flag_){
        try {
            loop_.remove_channel(listener_channel_);
        } catch(...) {}
    }
}

void Acceptor::set_new_connection_callback(NewConnectionCallback callback) {
    if(!callback) {
        throw std::invalid_argument("empty callback");
    }
    connection_callback_=std::move(callback);
}
void Acceptor::start() {
    if(start_flag_) {
        throw std::logic_error("restart");
    }
    if(!connection_callback_) {
        throw std::logic_error("no connection callback");
    }
    // listen
    if(::listen(listener_.get(),backlog_)==-1) {
        const int error_code=errno;
        system_error_helper(error_code,"listen");
    }
    // 让 listener_channel 关注 EPOLLIN
    listener_channel_.set_interest_events(EPOLLIN);
    // set channel 的 read_callback
    listener_channel_.set_read_callback([this]{this->handle_accept();});
    // 向 loop add channel
    loop_.add_channel(listener_channel_);
    start_flag_=true;
}

int Acceptor::listen_fd() const noexcept {
    return listener_.get();
}
std::uint16_t Acceptor::port() const noexcept {
    return port_;
}
bool Acceptor::listening() const noexcept {
    return start_flag_;
}

void Acceptor::handle_accept() {
    while(1) {
        UniqueFd connection(::accept4(listener_.get(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC));
        if(connection) {
            // 得到 connection,调用 connection callback
            connection_callback_(std::move(connection));
        } else if(errno==EINTR) {
            continue ;
        } else if(errno==EAGAIN||errno==EWOULDBLOCK) {
            // 成功榨干，当前没有立即能拿到的 pending connection 了
            // 结束本次 accept loop
            break ;
        } else {
            const int error_code=errno;
        system_error_helper(error_code,"accept4");
        }
    }
}