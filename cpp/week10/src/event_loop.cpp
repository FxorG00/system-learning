#include "event_loop.hpp"
#include <unistd.h>
#include <sys/epoll.h>
#include <iostream>
#include <chrono>
#include <system_error>

EventLoop::EventLoop() {
    const int epfd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epfd == -1) {
        throw std::system_error(errno,std::generic_category(),"epoll_create1");
    }
    epfd_=epfd;
}
EventLoop::~EventLoop() {
    if(::close(epfd_)==-1) {
        ::perror("close epfd_");
    }
}

void EventLoop::add_channel(Channel& channel) {
    epoll_event interest{};
    interest.events=channel.interest_events();
    const int fd=channel.fd();
    interest.data.fd=fd;
    if(::epoll_ctl(epfd_,EPOLL_CTL_ADD,fd,&interest)==-1) {
        throw std::system_error(errno,std::generic_category(),"epoll_ctl ADD");
    }
    map_[fd]=&channel;
}

void EventLoop::update_channel(Channel& channel) {
    epoll_event interest{};
    interest.events=channel.interest_events();
    const int fd=channel.fd();
    interest.data.fd=fd;
    if(::epoll_ctl(epfd_,EPOLL_CTL_MOD,fd,&interest)==-1) {
        throw std::system_error(errno,std::generic_category(),"epoll_ctl MOD");
    }
    map_[fd]=&channel;
}
void EventLoop::remove_channel(Channel& channel) {
    epoll_event interest{};
    interest.events=channel.interest_events();
    const int fd=channel.fd();
    interest.data.fd=fd;
    if(::epoll_ctl(epfd_,EPOLL_CTL_DEL,fd,&interest)==-1) {
        throw std::system_error(errno,std::generic_category(),"epoll_ctl DEL");
    }
    map_.erase(fd);
}

int EventLoop::poll_once(int timeout_ms) {
    const std::size_t returned_event_capacity=1024;
    epoll_event returned_event[returned_event_capacity];
    bool flag=(timeout_ms==-1);
    const auto begin=std::chrono::steady_clock::now();
    while(1) {
        const auto end=std::chrono::steady_clock::now();
        const auto elapsed = end - begin;
        const int elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
        );
        const int remained_ms=timeout_ms-elapsed_ms;
        if(!flag&&remained_ms<0) {
            return 0;
        }
        int ready_count=::epoll_wait(epfd_,returned_event,returned_event_capacity,flag?-1:remained_ms);
        if(ready_count>0) {
            for(std::size_t i=0;i<static_cast<std::size_t>(ready_count);i++) {
                int fd=returned_event[i].data.fd;
                // 设置 fd 对应的 channel 的ready_mask
                map_[fd]->set_ready_events(returned_event[i].events);
                // 调用对应 handle_event()
                map_[fd]->handle_event();
            }
            return ready_count;
        } else if(ready_count==0) {
            return 0;
        } else {
            if(errno==EINTR) {
                continue ;
            } else {
                throw std::system_error(errno,std::generic_category(),"epoll_wait");
            }
        }
    }
}