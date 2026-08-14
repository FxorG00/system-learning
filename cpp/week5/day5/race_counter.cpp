#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

int main() {
    int shared_interger_counter=0;
    int thread_count=4;
    int increments_per_thread=1000000;
    std::vector<std::thread>workers;
    auto task=[&shared_interger_counter,&increments_per_thread]{
        for(int i=1;i<=increments_per_thread;i++) {
            shared_interger_counter++;
        }
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