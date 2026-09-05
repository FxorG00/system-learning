#pragma once
#include <thread>
#include <fstream>
#include <string>
#include "blocking_queue.hpp"

class AsyncLogger {
public:
    AsyncLogger(std::string output_path, std::size_t capacity);
    ~AsyncLogger();
    bool log(std::string record);
    bool shutdown();
private:
    static void writer_work(AsyncLogger& logger,BlockingQueue<std::string>& queue,std::ofstream& output);
    BlockingQueue<std::string>queue;
    std::ofstream output;
    std::thread writer;
    bool write_failed;
};