#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <functional>

// 全局区 所有 thread 共享
// shared state
bool is_ready=false;
int value=0;
std::mutex mutex_lock;
std::condition_variable cv;

void worker_task() {
    std::unique_lock<std::mutex> ulock(mutex_lock);
    cv.wait(ulock,[]{return is_ready==true;});
    std::cout<<"worker task get value: "<<value<<std::endl;
}

void main_task() {
    // 如果要动 shared state 里的 value 
    // 应该先拿到 lock
    {
        std::lock_guard<std::mutex> guard(mutex_lock);
        // sleep(1);
        is_ready=true;
        value=2007;
    }
    cv.notify_all();
}

int main() {
    std::thread worker(worker_task);
    main_task();
    worker.join();
    return 0;
}