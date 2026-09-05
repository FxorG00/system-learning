#include "async_logger.hpp"
#include "thread_pool.hpp"
#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <future>
#include <fstream>

bool validate(std::vector<std::future<std::size_t> >& futures,std::size_t& task_count,std::string& output_path) {
    for(std::size_t i=0;i<task_count;i++) {
        try {
            // .get() 可能会抛出来 task 执行时的异常
            if(futures[i].get()!=i) {
                return false;
            }
        } catch(const std::exception& error) {
            std::cerr<<error.what()<<'\n';
            return false;
        }        
    }
    std::ifstream input(output_path);
    std::string line;
    std::map<std::string,std::size_t>mp;
    while(std::getline(input,line)) {
        ++mp[line];
    }
    if(mp.size()!=task_count) {
        return false;
    }
    for(std::size_t i=0;i<task_count;i++) {
        std::ostringstream builder;
        builder << "task: " << i;
        std::string result = builder.str();
        if(mp[result]!=1) {
            return false;
        }
    }
    return true;
}

bool work(std::size_t worker_count=4,std::size_t task_queue_capacity = 16,
std::size_t logger_queue_capacity = 8,std::size_t task_count = 100,
std::string output_path = "component_demo.log") {
    AsyncLogger logger(output_path,logger_queue_capacity);
    ThreadPool pool(worker_count,task_queue_capacity);
    // 异常退出的时候，我希望是先调用 pool 的析构，让 pool shutdown
    // 这样所有 task 才结束，不会再有对 logger 的提交了
    // 然后再让 logger 析构，去把 log queue 的 record output
    // 所以构造顺序是这样
    std::vector<std::future<std::size_t> >futures;
    futures.reserve(task_count);
    for(std::size_t i=0;i<task_count;i++) {
        auto future=pool.submit([&logger,i](){
            std::ostringstream builder;
            builder << "task: " << i;
            std::string result = builder.str();
            logger.log(result);
            return i;
        });
        // submit 仍然会阻塞等待直到成功才会执行下一条命令
        futures.push_back(std::move(future));
    }
    pool.shutdown();
    // 因为 pool.shutdown 会 join all workers
    // 所以到这里所有 tasks 执行结束，所以 records 都提交到 logger 里了
    if(!logger.shutdown()) {
        std::cout<<"FAIL\n";
        return false;
    }
    // 所以这里放心让 logger.shutdown() 去 output 到 file
    // 现在进入 validate
    if(!validate(futures,task_count,output_path)) {
        std::cout<<"FAIL\n";
        return false;
    }
    std::cout<<"PASS\n";
    return true;
}

int main() {
    if(!work()) {
        return 1;
    }
    return 0;
}