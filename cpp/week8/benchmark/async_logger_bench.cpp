#include "async_logger.hpp"
#include <string>
#include <sstream>
#include <chrono>
#include <map>
#include <mutex>
std::vector<std::string>str;
std::mutex output_mutex;
const std::string output_path="logger_api_demo.txt";
const std::size_t producer_count=4,async_logger_queue_capacity=100000;
std::size_t str_count=300000;
bool validate();
// prepare records
void init() {
    str.reserve(str_count);
    for(std::size_t i=0;i<str_count;i++) {
        std::ostringstream builder;
        builder << "log: " << i;
        std::string result = builder.str();
        str.push_back(std::move(result));
    }
    std::cout<<"record count: "<<str_count<<'\n';
    std::cout<<"producer count: "<<producer_count<<'\n';
    std::cout<<"async logger queue capacity: "<<async_logger_queue_capacity<<'\n';
}

// 同步 log
// 获取 ouput 对象前需要先拿到 mutex
void sync_log(std::string record,std::ofstream &output) {
    std::lock_guard<std::mutex> guard(output_mutex);
    output<<record<<'\n';
}

bool record_time_run_sync_logger() {
    std::vector<double>submission_time_vector,end_to_end_time_vector;
    std::cout<<"record_time_run_sync_logger: \n";
    for(std::size_t t=0;t<6;t++) {
        std::ofstream output(output_path,std::ios::out | std::ios::trunc);
        std::vector<std::thread>producers;
        producers.reserve(producer_count);
        std::size_t begin=0,base=str_count/producer_count,remainder=str_count%producer_count;
        std::condition_variable cv;
        std::mutex mutex;
        bool gate=false;
        for(std::size_t i=0;i<producer_count;i++) {
            std::size_t end=begin+base+(remainder>0);
            if(remainder>0) {
                --remainder;
            }
            producers.emplace_back([&cv,begin,end,&output,&gate,&mutex](){
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock,[&](){return gate;});
                // 此时会持有 gate 的 mutex，但是现在 gate 已经 true 了，所以我们并不需要这个 mutex 了
                // 因为我们接下来不需要再次访问 gate
                // 所以需要释放，避免影响其他 thread 观察 gate
                lock.unlock();
                for(std::size_t i=begin;i<end;i++) {
                    sync_log(str[i],output);
                }
            });
            begin=end;
        }
        // 先计时再打开闸门，确保在开始时刻前没有任何 thread 调用过 clog
        const auto time_begin=std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex>guard(mutex);
            gate=true;
            cv.notify_all();
        }
        for(auto& producer: producers) {
            producer.join();
        }
        // 此时每个 producer 都完成任务，也就是 call log 均成功
        const auto submission_end = std::chrono::steady_clock::now();
        // 此时保证每个 producer 都 log 成功
        // 才能关闭
        output.flush();
        output.close();
        const auto end_to_end_end=std::chrono::steady_clock::now();
        if(t==0) {
            // 作为 warm-up
            continue ;
        }
        const std::chrono::duration<double> submission_elapsed = submission_end - time_begin;
        const double submission_seconds = submission_elapsed.count();
        const std::chrono::duration<double> end_to_end_elapsed = end_to_end_end - time_begin;
        const double end_to_end_seconds = end_to_end_elapsed.count();
        std::cout<<"submission time: "<<submission_seconds<<"s ";
        std::cout<<"end-to-end time: "<<end_to_end_seconds<<"s ";
        submission_time_vector.push_back(submission_seconds);
        end_to_end_time_vector.push_back(end_to_end_seconds);
        if(validate()) {
            std::cout<<"PASS\n";
        } else {
            std::cout<<"FAIL\n";
            return false;
        }
    }
    sort(submission_time_vector.begin(),submission_time_vector.end());
    sort(end_to_end_time_vector.begin(),end_to_end_time_vector.end());
    std::cout<<"submission time: \n";
    std::cout<<"median: "<<submission_time_vector[submission_time_vector.size()/2]<<"s, min: "<<
    submission_time_vector[0]<<"s, max: "<<submission_time_vector.back()<<"s\n";

    std::cout<<"end-to-end time: \n";
    std::cout<<"median: "<<end_to_end_time_vector[end_to_end_time_vector.size()/2]<<"s, min: "<<
    end_to_end_time_vector[0]<<"s, max: "<<end_to_end_time_vector.back()<<"s\n";
    std::cout<<"records per second: "<<1.0*str_count/end_to_end_time_vector[end_to_end_time_vector.size()/2]<<'\n';
    return true;
}

bool validate() {
    std::ifstream input("logger_api_demo.txt");
    std::string line;
    std::map<std::string,std::size_t>mp;
    while(std::getline(input,line)) {
        ++mp[line];
    }
    if(mp.size()!=str_count) return false;
    for(std::size_t i=0;i<str_count;i++) {
        std::ostringstream builder;
        builder << "log: " << i;
        std::string result = builder.str();
        if(mp[result]!=1) {
            return false;
        }
    }
    return true;
}

bool record_time_run_async_logger() {
    std::vector<double>submission_time_vector,end_to_end_time_vector;
    std::cout<<"record_time_run_async_logger: \n";
    for(std::size_t t=0;t<6;t++) {
        std::vector<std::thread>producers;
        producers.reserve(producer_count);
        AsyncLogger logger(output_path,async_logger_queue_capacity);
        std::size_t begin=0,base=str_count/producer_count,remainder=str_count%producer_count;
        std::condition_variable cv;
        std::mutex mutex;
        bool gate=false;
        for(std::size_t i=0;i<producer_count;i++) {
            std::size_t end=begin+base+(remainder>0);
            if(remainder>0) {
                --remainder;
            }
            producers.emplace_back([&cv,begin,end,&gate,&mutex,&logger]{
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock,[&](){return gate;});
                // 此时会持有 gate 的 mutex，但是现在 gate 已经 true 了，所以我们并不需要这个 mutex 了
                // 所以需要释放，避免影响其他 thread 观察 gate
                lock.unlock();
                for(std::size_t i=begin;i<end;i++) {
                    if(!logger.log(str[i])) {
                        exit(1);
                    }
                }
            });
            begin=end;
        }
        // 先计时再打开闸门，确保在开始时刻前没有任何 thread 调用过 clog
        const auto time_begin=std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex>guard(mutex);
            gate=true;
            cv.notify_all();
        }
        for(auto& producer: producers) {
            producer.join();
        }
        // 此时保证每个 producer 都成功调用 log
        // 然后 log 是会阻塞等待的，执行结束意味着成功把 record push 进去 queue
        const auto submission_end = std::chrono::steady_clock::now();
        if(!logger.shutdown()) {
            return false;
        }
        if(t==0) {
            // 作为 warm-up
            continue ;
        }
        // shutdown logger；到这里的时候，所有的 record 成功 output，因为 queue 都 drain 干净了
        const auto end_to_end_end=std::chrono::steady_clock::now();
        const std::chrono::duration<double> submission_time = submission_end - time_begin;
        const double submission_time_seconds = submission_time.count();
        const std::chrono::duration<double> end_to_end_time = end_to_end_end - time_begin;
        const double end_to_end_time_seconds = end_to_end_time.count();
        std::cout<<"submission time: "<<submission_time_seconds<<" s ";
        std::cout<<"end-to-end time: "<<end_to_end_time_seconds<<" s ";
        submission_time_vector.push_back(submission_time_seconds);
        end_to_end_time_vector.push_back(end_to_end_time_seconds);
        if(validate()) {
            std::cout<<"PASS\n";
        } else {
            std::cout<<"FAIL\n";
            return false;
        }
    }
    sort(submission_time_vector.begin(),submission_time_vector.end());
    sort(end_to_end_time_vector.begin(),end_to_end_time_vector.end());
    std::cout<<"submission time: \n";
    std::cout<<"median: "<<submission_time_vector[submission_time_vector.size()/2]<<"s, min: "<<
    submission_time_vector[0]<<"s, max: "<<submission_time_vector.back()<<"s\n";

    std::cout<<"end-to-end time: \n";
    std::cout<<"median: "<<end_to_end_time_vector[end_to_end_time_vector.size()/2]<<"s, min: "<<
    end_to_end_time_vector[0]<<"s, max: "<<end_to_end_time_vector.back()<<"s\n";
    std::cout<<"records per second: "<<1.0*str_count/end_to_end_time_vector[end_to_end_time_vector.size()/2]<<'\n';
    return true;
}

int main() {
    init();
    if(!record_time_run_sync_logger()) {
        return 1;
    }
    if(!record_time_run_async_logger()) {
        return 1;
    }
    return 0;
}