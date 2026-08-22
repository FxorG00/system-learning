#pragma once
#include <iostream>
#include <cstddef>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <optional>
template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity):capacity_(capacity),closed_(false) {
        if(capacity==0) {
            throw std::invalid_argument("invalid argument: queue capacity=0");
        }
    }
    // deleted copy constructor / copy assignment
    BlockingQueue(const BlockingQueue& other)=delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;
    void close() {
        // 更改 queue 的 status，先拿锁
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // 拿到锁了就可以改了
        if(closed_) {
            // 并非首次 close
            return ;
        }
        // 首次 close
        closed_=true;
        // notify_all 所有 waiter，也就是不让任何 worker sleeping 了，都需要来处理
        not_empty_cv_.notify_all();
        not_full_cv_.notify_all();
        // RAII，不管 lock
    }
    bool push(T value) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // 要等待 queue not full
        not_full_cv_.wait(lock,[&]{return closed_||q_.size()<capacity_;});
        // 现在持有 queue_mutex_ 
        if(closed_) {
            // queue 已经关闭，则 push 失败
            // unique_lock 是 RAII，不用去管 lock
            return false;
        }
        // queue not empty
        q_.push(std::move(value));
        lock.unlock();
        not_empty_cv_.notify_one();
        // 通知在 pop 里面等待 not_empty_cv wake up 的 waiter
        return true;
    }
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        not_empty_cv_.wait(lock,[&]{return closed_||!q_.empty();});
        // 现在持有 queue_mutex
        if(closed_) {
            // 已经 closed 了，判断是否 not empty
            if(!q_.empty()) {
                // 非空的话还能取出来
                T value=std::move(q_.front()); q_.pop();
                return value;
            } else {
                // 已经为空则 pop 失败
                return std::nullopt;
            }
        }
        // 并非 closed，则 q not empty
        T value=std::move(q_.front()); q_.pop();
        lock.unlock();
        not_full_cv_.notify_one();
        return value;
    }
private: 
    // closed_ 为 true 则 queue 已经 closed
    std::size_t capacity_;
    bool closed_;
    std::queue<T>q_;
    std::mutex queue_mutex_;
    std::condition_variable not_empty_cv_,not_full_cv_;
};