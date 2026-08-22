#include <vector>
#include <map>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstddef>
#include <algorithm>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <optional>
#include <atomic>

void work(std::size_t increments,std::mutex& mutex,std::size_t& mutex_counter,std::atomic<std::size_t>& counter) {
    for(std::size_t i=0;i<increments;i++) {
        counter.fetch_add(1);
    }
    for(std::size_t i=0;i<increments;i++) {
        std::lock_guard<std::mutex>guard(mutex);
        ++mutex_counter;
    }
}

bool test(std::size_t worker_count,std::size_t increments) {
    std::vector<std::thread>workers;
    std::mutex mutex;
    std::size_t mutex_counter=0;
    std::atomic<std::size_t>counter=0;
    workers.reserve(worker_count);
    for(std::size_t i=0;i<worker_count;i++) {
        workers.emplace_back(work,increments,std::ref(mutex),std::ref(mutex_counter),std::ref(counter));
    }
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
    }
    std::size_t expected_total=worker_count*increments;
    std::cout<<"expected total: "<<expected_total<<'\n';
    std::cout<<"mutex counter result: "<<mutex_counter<<'\n';
    std::cout<<"atomic counter result: "<<counter<<'\n';
    if(mutex_counter==expected_total&&counter==expected_total) {
        std::cout<<"PASS\n";
        return true;
    } else {
        std::cout<<"FAIL\n";
        return false;
    }
}

int main() {
    if(!test(1,0)) {
        return 1;
    }    
    if(!test(1,10000)) {
        return 1;
    }    
    if(!test(4,10000)) {
        return 1;
    }    
    if(!test(8,10000)) {
        return 1;
    }    
    return 0;
}