#pragma once

#include <cstdint>
#include <functional>
#include <sys/epoll.h>

// Channel 用于保存一个 fd 的信息
// 描述它的 interest_mask 与 ready_mask
// 以及对应 event 过来时需要调用的 callback
class Channel {
public:
    using Callback = std::function<void()>;

    explicit Channel(int fd) noexcept;

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    int fd() const noexcept;

    std::uint32_t interest_events() const noexcept;
    void set_interest_events(std::uint32_t events) noexcept;
    void add_interest_events(std::uint32_t events) noexcept;
    void del_interest_events(std::uint32_t events) noexcept;
    std::uint32_t ready_events() const noexcept;
    void set_ready_events(std::uint32_t events) noexcept;

    void set_read_callback(Callback callback);
    void set_write_callback(Callback callback);
    void set_error_callback(Callback callback);

    void handle_event();

private:
    // Round1：由你设计 representation。
    int fd_=0;
    std::uint32_t interest_mask_=0,ready_mask_=0;
    Callback read_callback_,write_callback_,error_callback_;
};