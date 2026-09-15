#include "connection.hpp"
#include <fcntl.h>
#include <system_error>
#include <sys/socket.h>
#include <iostream>
void Connection::system_error_helper(int error_code,std::string msg) {
    throw std::system_error(error_code,std::generic_category(),msg);
}

void Connection::close_helper() {
    if(!close_flag_) {
        close_flag_=1;
        close_callback_(connection_.get());
    }
}

// 接管 connected socket 的 ownership；不得复制同一个 fd owner。
// 并且去构造 channel,buffer
Connection::Connection(EventLoop& loop, UniqueFd socket):
loop_(loop),connection_(std::move(socket)),connection_channel_(connection_.get()) {
    if(!connection_) {
        throw std::invalid_argument("Connection requires a valid socket");
    }
    const int status_flags = ::fcntl(connection_.get(), F_GETFL);
    if(status_flags==-1) {
        const int error_code=errno;
        system_error_helper(error_code,"fcntl(F_GETFL)");
    }
    const bool nonblocking =(status_flags & O_NONBLOCK) != 0;
    if(!nonblocking) {
        throw std::invalid_argument("Connection requires a nonblocking socket");
    }
}
Connection::~Connection() noexcept {
    if(start_flag_) {
        try {
            loop_.remove_channel(connection_channel_);
        } catch(...) {}
    }
}

void Connection::set_message_callback(MessageCallback callback) {
    if(!callback) {
        throw std::invalid_argument("MessageCallback must not be empty");
    }
    if(start_flag_) {
        // start 后再设置 callback，抛异常
        throw std::logic_error("Connection callbacks cannot be changed after start");
    }
    message_callback_=std::move(callback);
}
void Connection::set_close_callback(CloseCallback callback) {
    if(!callback) {
        throw std::invalid_argument("CloseCallback must not be empty");
    }
    if(start_flag_) {
        // start 后再设置 callback，抛异常
        throw std::logic_error("Connection callbacks cannot be changed after start");
    }
    close_callback_=std::move(callback);
}

void Connection::handle_error() {
    int socket_error = 0;
    socklen_t length = sizeof(socket_error);
    if (::getsockopt(connection_.get(),SOL_SOCKET,SO_ERROR,&socket_error,&length) == -1) {
        const int error_code=errno;
        close_helper();
        system_error_helper(error_code,"getsockopt(SO_ERROR)");
    } else if (socket_error != 0) {
        // std::cerr << "socket error: " << std::strerror(socket_error) << '\n';
        close_helper();
        system_error_helper(socket_error,"socket SO_ERROR");
    }
}

// 把 Channel 注册到 EventLoop；同一对象只能成功 start 一次。
void Connection::start() {
    if(!close_callback_||!message_callback_) {
        throw std::logic_error("Connection callbacks must be set before start");
    }
    if(start_flag_) {
        throw std::logic_error("Connection is already started");
    }
    // 让 connection_channel 关注 EPOLLIN,EPOLLRDHUP
    connection_channel_.set_interest_events(EPOLLIN|EPOLLRDHUP);
    // set channel 的 read_callback
    connection_channel_.set_read_callback([this]{this->handle_recv();});
    connection_channel_.set_write_callback([this]{this->handle_send();});
    connection_channel_.set_error_callback([this]{this->handle_error();});
    // 向 loop add channel
    loop_.add_channel(connection_channel_);
    start_flag_=true;
}

void Connection::send(const char* data, std::size_t length) {
    if(data==nullptr&&length>0) {
        throw std::invalid_argument("Connection::send data must not be null when length is positive");
    }
    if(data==nullptr||length==0) {
        return ;
    }
    if(!start_flag_) {
        throw std::logic_error("Connection::send requires a started connection");    
    }
    if(close_flag_) {
        throw std::logic_error("Connection::send called after close request");
    }
    // 直接把 data append 到 output
    output_.append(data,length);
    handle_send();
}

void Connection::try_update_channel() {
    try {
        loop_.update_channel(connection_channel_);
    } catch(...) {
        close_helper();
        throw ;
    }
}

void Connection::handle_send() {
    std::size_t offset=0; // 本次已经 send 的 output 的 bytes 数量
    while(offset<output_.readable_bytes()) {
        const ssize_t n=::send(connection_.get(),output_.peek()+offset,output_.readable_bytes()-offset,MSG_NOSIGNAL);
        if(n>0) {
            // 发送成功
            offset+=static_cast<std::size_t>(n);
        } else if(n<0) {
            if(errno==EINTR) {
                continue ;
            } else if(errno==EAGAIN||errno==EWOULDBLOCK) {
                // 本次没办法再 send 了
                // 需要 retrieve 已经 send 的 bytes
                output_.retrieve(offset);
                if(!output_.empty()) {
                    connection_channel_.add_interest_events(EPOLLOUT);
                    try_update_channel();
                }
                return ;
            } else {
                // 其他错误
                // 保存当下 `errno`，先提交 close request，再抛 `std::system_error`
                const int error_code=errno;
                close_helper();
                system_error_helper(error_code,"send");
            }
        }
    }
    // 到这里一定是 output_ 都发完了
    output_.retrieve(output_.readable_bytes());
    connection_channel_.del_interest_events(EPOLLOUT);
    try_update_channel();
    if(peer_write_closed_flag_ && output_.empty()) {
        close_helper();
    }
}

int Connection::fd() const noexcept {
    return connection_.get();
}
bool Connection::started() const noexcept {
    return start_flag_;
}
bool Connection::peer_write_closed() const noexcept {
    return peer_write_closed_flag_;
}
std::size_t Connection::pending_input_bytes() const noexcept {
    return input_.readable_bytes();
}
std::size_t Connection::pending_output_bytes() const noexcept {
    return output_.readable_bytes();
}

void Connection::try_message_callback(bool recv_flag) {
    if(recv_flag) {
        try {
            message_callback_(*this,input_);
        } catch(...) {
            close_helper();
            throw ;
        }
    }
}

// 在能 recv 的情况下，一直 recv，直到遇到边界。
// 把每次 recv 得到的 n bytes 都加入到 connection 的 input
void Connection::handle_recv() {
    char buffer[1024];
    bool recv_flag=false;
    while(1) {
        const ssize_t n = ::recv(connection_.get(), buffer, sizeof(buffer), 0);
        if (n > 0) {
            recv_flag=true;
            input_.append(buffer,static_cast<std::size_t>(n));
        } else if (n == 0) {  
            // 记录 peer EOF；output 为空时请求关闭，否则继续 drain output
            // peer write closed，但是我也许还有需要 send 的数据
            try_message_callback(recv_flag);
            peer_write_closed_flag_=true;
            if(output_.empty()) {
                close_helper();
            }
            return ;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 本轮暂时不能继续，保留 state 等下一次 readiness
            try_message_callback(recv_flag);
            return ;
        } else if(errno!=EINTR) {
            // 保存当下 `errno`，先提交 close request，再抛 `std::system_error`
            const int error_code=errno;
            close_helper();
            system_error_helper(error_code,"recv");
        }
    }
}