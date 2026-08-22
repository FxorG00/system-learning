#include <chrono>
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
#include <atomic>
using TestFunction = bool (*)(std::size_t, std::size_t);

void workA(std::mutex& mutex,std::size_t& counter,std::size_t iterations) {
    for(std::size_t i=0;i<iterations;i++) {
        std::lock_guard<std::mutex> guard(mutex);
        ++counter;
    }
}

bool testA(std::size_t worker_count,std::size_t iterations) {
    std::mutex mutex;
    std::vector<std::thread> workers;
    std::size_t counter=0;
    workers.reserve(worker_count);
    for(std::size_t i=0;i<worker_count;i++) {
        workers.emplace_back(workA,std::ref(mutex),std::ref(counter),iterations);
    }
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
    }
    bool success=(counter==worker_count*iterations);
    return success;
}

void workB(std::size_t index,std::vector<std::size_t>& counters,std::size_t iterations) {
    for(std::size_t i=0;i<iterations;i++) {
        ++counters[index];
    }
}

bool testB(std::size_t worker_count,std::size_t iterations) {
    std::vector<std::size_t>counters(worker_count);
    std::vector<std::thread>workers;
    workers.reserve(worker_count);
    for(std::size_t i=0;i<worker_count;i++) {
        workers.emplace_back(workB,i,std::ref(counters),iterations);
    }
    std::size_t sum=0;
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
        sum+=counters[i];
    }
    return sum==worker_count*iterations;
}

void workC(std::size_t index,std::vector<std::atomic<std::size_t> >& counters,std::size_t iterations) {
    for(std::size_t i=0;i<iterations;i++) {
        counters[index].fetch_add(1);
    }
}

bool testC(std::size_t worker_count,std::size_t iterations) {
    // C 就是一个 false sharing
    // 虽然每个 thread 操作的都不是同一个 atomic object
    // 但是他们很可能在同一个 cache line 导致 cache 的 ownership 的转让
    // 以及每次操作后可能使得很多 core 的 cache line invalid
    // 从而性能上可能不优
    std::vector<std::atomic<std::size_t> >counters(worker_count);
    for(std::size_t i=0;i<counters.size();i++) {
        counters[i].store(0);
    }
    std::vector<std::thread>workers;
    workers.reserve(worker_count);
    for(std::size_t i=0;i<worker_count;i++) {
        workers.emplace_back(workC,i,std::ref(counters),iterations);
    }
    std::size_t sum=0;
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
        sum+=counters[i].load();
    }
    return sum==worker_count*iterations;
}

#if defined(__cpp_lib_hardware_interference_size)
constexpr std::size_t kDestructiveSize =
    std::hardware_destructive_interference_size;
#else
constexpr std::size_t kDestructiveSize = 64;
#endif

struct alignas(kDestructiveSize) PaddedCounter {
    std::atomic<std::size_t> value{0};
};

void workD(std::size_t index,std::vector<PaddedCounter>& counters,std::size_t iterations) {
    for(std::size_t i=0;i<iterations;i++) {
        counters[index].value.fetch_add(1);
    }
}

bool testD(std::size_t worker_count,std::size_t iterations) {
    // D 让每个 atomic 尽可能不在同一个 cache line
    // 以降低 每个 core 不断竞争某个 line 的 ownership
    std::vector<PaddedCounter>counters(worker_count);
    std::vector<std::thread>workers;
    workers.reserve(worker_count);
    for(std::size_t i=0;i<worker_count;i++) {
        workers.emplace_back(workD,i,std::ref(counters),iterations);
    }
    std::size_t sum=0;
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
        sum+=counters[i].value.load();
    }
    return sum==worker_count*iterations;
}


void test(std::string name,std::size_t worker_count,std::size_t iterations,std::size_t repeat_count,TestFunction func) {
    std::vector<std::int64_t>time_vector;
    std::int64_t sum_time=0;
    std::cout<<"test "<<name<<":\n";
    for(std::size_t t=1;t<=repeat_count;t++) {
        const auto start = std::chrono::steady_clock::now();
        bool success=func(worker_count,iterations);
        const auto stop = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        
        std::cout<<"repeat "<<t<<": "<<elapsed.count()<<" us,";
        if(success) {
            time_vector.push_back(elapsed.count());
            sum_time+=elapsed.count();
            std::cout<<" PASS\n";
        } else {
            std::cout<<" FAIL\n";
        }
    }
    if(!time_vector.empty()) {
        sort(time_vector.begin(),time_vector.end());
        std::cout<<"median: "<<time_vector[time_vector.size()/2]<<" us\n";
        std::cout<<"average: "<<1.0*sum_time/time_vector.size()<<" us\n";
    }
    std::cout<<'\n';
}

int main() {
    test("A",1,0,10,testA);
    test("B",1,0,10,testB);
    test("C",1,0,10,testC);
    test("D",1,0,10,testD);

    test("A",1,100000,10,testA);
    test("B",1,100000,10,testB);
    test("C",1,100000,10,testC);
    test("D",1,100000,10,testD);

    test("A",2,1000000,10,testA);
    test("B",2,1000000,10,testB);
    test("C",2,1000000,10,testC);
    test("D",2,1000000,10,testD);

    test("A",4,1000000,10,testA);
    test("B",4,1000000,10,testB);
    test("C",4,1000000,10,testC);
    test("D",4,1000000,10,testD);
}