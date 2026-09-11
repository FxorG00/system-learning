#pragma once
#include "channel.hpp"
#include <map>
#include <unistd.h>
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
#include <system_error>

class Channel;

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    void add_channel(Channel& channel);
    void update_channel(Channel& channel);
    void remove_channel(Channel& channel);

    int poll_once(int timeout_ms);

private:
    // Round1：由你决定 V1 所需的 private representation。
    int epfd_=0;
    std::map<int,Channel*>map_;
};