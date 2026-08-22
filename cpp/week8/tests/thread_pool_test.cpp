#include "thread_pool.hpp"

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

int get_value(int x,int y) {
    return 2*x*y;
}

int main() {
    try {
        ThreadPool my_threadpool(1,1);
        my_threadpool.shutdown();
        auto future=my_threadpool.submit(get_value,3,4);
        std::cout<<future.get()<<'\n';
    } catch(const std::exception& error) {
        std::cerr<<error.what();
        return 1;
    }
    return 0;
}