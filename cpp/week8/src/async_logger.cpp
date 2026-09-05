#include "async_logger.hpp"
AsyncLogger::AsyncLogger(std::string output_path, std::size_t capacity)
:queue(capacity),output(output_path,std::ios::out | std::ios::trunc),write_failed(false) {
    if(!output.is_open()) {
        // 会抛异常
        // 那么会先反向顺序析构完已经构造成功的 queue,output
        // 然后向外抛异常
        throw std::runtime_error("error: output is not open!");
    }
    // 此时已经确保 output 能正常打开
    writer=std::thread(writer_work,std::ref(*this),std::ref(queue),std::ref(output));
    // 如果创建 thread 的时候抛异常，那么该 thread object 自身构造失败，不会调用析构函数
    // 会先栈展开，去析构 output,queue，然后这个 std::system_error 会向外抛出
}
AsyncLogger::~AsyncLogger() {
    shutdown();
}
bool AsyncLogger::log(std::string record) {
    return queue.push(std::move(record));
}
bool AsyncLogger::shutdown() {
    queue.close();
    if(writer.joinable()) {
        writer.join();
    }
    return !write_failed;
}

void AsyncLogger::writer_work(AsyncLogger& logger,BlockingQueue<std::string>& queue,std::ofstream& output) {
    while(1) {
        std::optional<std::string> value=queue.pop();
        if(value) {
            // 要求把 records 写成一行一条，所以我们要加 \n
            output<<(*value)<<'\n';
            if(!output) {
                // 只有 writer 去写 write_failed，并且在 writer join 后才会读，所以不需要 mutex
                logger.write_failed=true;
            }
        } else {
            break ;
        }
    }
    // 我们把操作 ouput 的事情，包括 flush,close 都交给 writer
    // 让 ownership 更简单
    // 刷新缓冲区
    output.flush();
    if(!output) {
        // flush 失败则在 shutdown return 的时候也需要报告
        logger.write_failed=true;
    }
    // 关闭
    output.close();
    if(!output) {
        logger.write_failed=true;
    }
}