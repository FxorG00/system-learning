#include <thread>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

void work(std::vector<std::int64_t>& sum,const std::vector<int>& values,std::size_t index,std::size_t begin,std::size_t end) {
    for(std::size_t i=begin;i<end;i++) {
        sum[index]+=values[i];
    }
}

// flag=true: 主动抛异常测试
bool parallel_sum(const std::vector<int>& values,std::size_t requested_worker_count,bool flag) {
    std::vector<std::thread> workers;
    std::size_t element_count=values.size();
    if(values.size()==0) {
        std::cout<<"success\n";
        return true;
    }
    std::size_t worker_count=std::min(requested_worker_count, values.size());
    if(worker_count==0) {
        std::cerr<<"worker count =0\n";
        return false;
    }
    std::size_t base=element_count/worker_count,remainder=element_count%worker_count;
    std::size_t begin=0;
    workers.reserve(worker_count);
    std::vector<std::int64_t> sum;
    sum.resize(worker_count);
    try {
        for(std::size_t i=0;i<worker_count;i++) {
            if(flag&&i==2) {
                throw std::runtime_error("simulated worker creation failure");
            }
            std::size_t end=begin+base+(remainder>0);
            workers.emplace_back(work,std::ref(sum),std::ref(values),i,begin,end);
            if(remainder>0) {
                --remainder;
            }
            begin=end;
        }
    } catch (const std::exception& error) {
        for(std::thread& worker: workers) {
            if(worker.joinable()) {
                worker.join();
            }
        }
        std::cerr<<"create worker failed: "<<error.what()<<'\n';
        return false;
    }
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
    }
    std::int64_t worker_sum=0;
    for(std::size_t i=0;i<worker_count;i++) {
        worker_sum+=sum[i];
    }
    std::int64_t main_sum=0;
    for(std::size_t i=0;i<values.size();i++) {
        main_sum+=values[i];
    }
    if(worker_sum==main_sum) {
        std::cout<<"success\n";
        return true;
    } else {
        std::cout<<"failed\n";
        return false;
    }
}

int main() {
    return 0;
}