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
template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(std::size_t capacity):capacity_(capacity) {
        if(capacity==0) {
            throw std::invalid_argument("capacity=0");
        }
    }
    // deleted copy constructor / copy assignment
    BlockingQueue(const BlockingQueue& other)=delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;
    void push(T value) {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        // 要等待 queue not full
        not_full_cv_.wait(lock,[&]{return q_.size()<capacity_;});
        // 现在持有 queue_mutex_ 且 queue not full 为 true
        q_.push(std::move(value));
        lock.unlock();
        not_empty_cv_.notify_all();
        // 通知在 pop 里面等待 not_empty_cv wake up 的 waiter
    }
    T pop() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        not_empty_cv_.wait(lock,[&]{return !q_.empty();});
        // 现在持有 queue_mutex 且 queue not empty
        T value=q_.front(); q_.pop();
        lock.unlock();
        not_full_cv_.notify_all();
        return value;
    }
private: 
    std::size_t capacity_;
    std::queue<T>q_;
    std::mutex queue_mutex_;
    std::condition_variable not_empty_cv_,not_full_cv_;
};