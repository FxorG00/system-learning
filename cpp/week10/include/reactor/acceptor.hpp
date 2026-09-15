#pragma once

#include "event_loop.hpp"
#include "unique_fd.hpp"
#include "channel.hpp"

#include <cstdint>
#include <functional>

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(UniqueFd)>;

    Acceptor(EventLoop& loop, std::uint16_t port, int backlog = 128);
    ~Acceptor();

    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;
    Acceptor(Acceptor&&) = delete;
    Acceptor& operator=(Acceptor&&) = delete;

    void set_new_connection_callback(NewConnectionCallback callback);
    void start();

    int listen_fd() const noexcept;
    std::uint16_t port() const noexcept;
    bool listening() const noexcept;

private:
    // Round1: design the representation yourself.
    EventLoop& loop_;
    UniqueFd listener_{-1};
    Channel listener_channel_{-1};
    bool start_flag_=0;
    std::uint16_t port_=0;
    int backlog_=0;
    NewConnectionCallback connection_callback_;
    // Channel invokes this private entry when the listener is readable.
    void handle_accept();
};