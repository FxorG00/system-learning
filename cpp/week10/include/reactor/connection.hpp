#pragma once
#include <cstddef>
#include <functional>
#include "channel.hpp"
#include "buffer.hpp"
#include "event_loop.hpp"
#include <string>
#include "unique_fd.hpp"
class Connection {
public:
    // Application 在 input Buffer 增长后检查并消费完整 messages。
    using MessageCallback = std::function<void(Connection&, Buffer&)>;

    // Connection 只请求关闭；真正 owner 根据 fd 找到并销毁对象。
    using CloseCallback = std::function<void(int)>;

    // 接管 connected socket 的 ownership；不得复制同一个 fd owner。
    Connection(EventLoop& loop, UniqueFd socket);
    ~Connection() noexcept;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;

    void set_message_callback(MessageCallback callback);
    void set_close_callback(CloseCallback callback);

    // 把 Channel 注册到 EventLoop；同一对象只能成功 start 一次。
    void start();

    // 调用返回后不再依赖 caller memory；保持多次调用的 byte order。
    // 尚未交给 kernel 的 suffix 必须由 Connection 自己保存。
    void send(const char* data, std::size_t length);

    int fd() const noexcept;
    bool started() const noexcept;
    bool peer_write_closed() const noexcept;
    std::size_t pending_input_bytes() const noexcept;
    std::size_t pending_output_bytes() const noexcept;
private:
    EventLoop& loop_;
    UniqueFd connection_;
    Channel connection_channel_;
    Buffer input_,output_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    bool start_flag_=0,peer_write_closed_flag_=0;
    bool close_flag_=0;
    void handle_recv();
    void handle_send();
    void handle_error();
    void close_helper();
    void system_error_helper(int error_code,std::string msg);
    void try_message_callback(bool recv_flag);
    void try_update_channel();
};