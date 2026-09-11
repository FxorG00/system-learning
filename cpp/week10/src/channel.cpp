#include "channel.hpp"
#include <utility>

Channel::Channel(int fd)noexcept:fd_(fd) {
    
}

int Channel::fd() const noexcept {
    return fd_;
}

std::uint32_t Channel::interest_events() const noexcept {
    return interest_mask_;
}
void Channel::set_interest_events(std::uint32_t events) noexcept {
    interest_mask_=events;
}

std::uint32_t Channel::ready_events() const noexcept {
    return ready_mask_;
}
void Channel::set_ready_events(std::uint32_t events) noexcept {
    ready_mask_=events;
}

void Channel::set_read_callback(Callback callback) {
    read_callback_=std::move(callback);
}
void Channel::set_write_callback(Callback callback) {
    write_callback_=std::move(callback);
}
void Channel::set_error_callback(Callback callback) {
    error_callback_=std::move(callback);
}

void Channel::handle_event() {
    if(ready_mask_&EPOLLIN) {
        if(read_callback_) {
            read_callback_();
        }
    }
    if(ready_mask_&EPOLLOUT) {
        if(write_callback_) {
            write_callback_();
        }
    }
    if(ready_mask_&EPOLLERR) {
        if(error_callback_) {
            error_callback_();
        }
    }
}