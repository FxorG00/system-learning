#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <functional>
int var=10;
int* p=new int(3);

void task(int i,std::mutex& mutex_lock) {
    mutex_lock.lock();
    std::cout<<"worker "<<i<<'\n';
    std::cout<<"PID: "<<::getpid()<<'\n';
    std::cout<<"Linux TID: "<<::syscall(SYS_gettid)<<'\n';
    std::cout<<"std::thread::id "<<std::this_thread::get_id()<<'\n';
    std::cout<<"global object address: "<<static_cast<void*>(&var)<<'\n';
    std::cout<<"shared heap object address: "<<static_cast<void*>(p)<<'\n';
    int tmp=10;
    std::cout<<"thread local variable address: "<<static_cast<void*>(&tmp)<<'\n';
    mutex_lock.unlock();
    sleep(20);
}

void main_task(std::mutex& mutex_lock) {
    mutex_lock.lock();
    std::cout<<"main "<<'\n';
    std::cout<<"PID: "<<::getpid()<<'\n';
    std::cout<<"Linux TID: "<<::syscall(SYS_gettid)<<'\n';
    std::cout<<"std::thread::id "<<std::this_thread::get_id()<<'\n';
    std::cout<<"global object address: "<<static_cast<void*>(&var)<<'\n';
    std::cout<<"shared heap object address: "<<static_cast<void*>(p)<<'\n';
    int tmp=10;
    std::cout<<"thread local variable address: "<<static_cast<void*>(&tmp)<<'\n';
    mutex_lock.unlock();
    sleep(20);
}

int main() {
    int thread_count=3;
    std::vector<std::thread>workers;
    std::mutex mutex_lock;
    for(int i=1;i<=thread_count;i++) {
        workers.emplace_back(task,i,std::ref(mutex_lock));
    }
    main_task(mutex_lock);
    for(std::size_t i=0;i<workers.size();i++) {
        workers[i].join();
    }
    return 0;
}