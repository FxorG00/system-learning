#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
    int shared_interger_counter=0;
    int thread_count=4;
    int increments_per_thread=1000000;
    std::mutex counter_mutex;
    std::vector<std::thread>workers;
    auto task=[&shared_interger_counter,&increments_per_thread,&counter_mutex]{
        std::lock_guard<std::mutex>guard(counter_mutex);
        // 这样如果拿不到 counter_mutex 这把锁，就不会接着往下执行
        // 只有目前拿得到 mutex 的 thread 会执行 ++，其他会等待拿锁

        for(int i=1;i<=increments_per_thread;i++) {
            shared_interger_counter++;
        }
        // task 执行结束,RAII 自动释放 lock
    };
    for(int i=1;i<=thread_count;i++) {
        workers.emplace_back(task);
    }
    for(std::size_t i=0;i<workers.size();i++) {
        workers[i].join();
    }
    std::cout<<"expected: "<<thread_count*increments_per_thread<<'\n';
    std::cout<<"actual: "<<shared_interger_counter<<'\n';
    std::cout<<std::flush;
    return 0;
}