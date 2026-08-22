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

void work(std::size_t index,std::atomic<int>& max_value,std::vector<std::optional<int> >& single_max,const std::vector<int>& input,std::size_t begin,std::size_t end) {
    for(std::size_t i=begin;i<end;i++) {
        if(!single_max[index]) {
            single_max[index]=input[i];
        } else {
            *single_max[index]=std::max(*single_max[index],input[i]);
        }
        while(1) {
            int expected=max_value.load();
            // 我期望，我在 CAS 的时候 max_value 的值应该等于此刻的观察值
            // 这样能证明，在中间没有其他 thread 插入操作
            if(input[i]<=expected) {
                // 因为中间就算插入操作，max_value 只会变大
                // 此时不可能有贡献
                break ;
            }
            // 到这里，意味着 input[i]>expected
            // 如果 expected 没有被改变，那么我的修改完全是需要进行的，也就是取 max 这一步对的
            // 被改变的话，可以不理他，接着 loop，因为后面 expected 会重新获取 max_value snapshot
            const bool changed=max_value.compare_exchange_weak(expected,input[i]);
            if(changed) {
                break ;
            }
        }
    }
}

bool test(const std::vector<int>& input,std::size_t worker_count) {
    if(input.empty()) {
        std::cerr<<"empty input\n";
        return false;
    }
    std::vector<std::optional<int> >single_max(worker_count);
    std::vector<std::thread>workers;
    workers.reserve(worker_count);
    std::atomic<int> max_value(input[0]);
    std::size_t base=input.size()/worker_count,remainder=input.size()%worker_count;
    std::size_t begin=0;
    for(std::size_t i=0;i<worker_count;i++) {
        std::size_t end=begin+base+(remainder>0);
        if(remainder>0) {
            --remainder;
        }
        workers.emplace_back(work,i,std::ref(max_value),std::ref(single_max),std::ref(input),begin,end);
        begin=end;
    }
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
    }
    bool pass_flag=true;
    base=input.size()/worker_count,remainder=input.size()%worker_count;
    begin=0;
    for(std::size_t i=0;i<worker_count;i++) {
        std::size_t end=begin+base+(remainder>0);
        if(remainder>0) {
            --remainder;
        }
        if(single_max[i]) {
            std::cout<<"thread "<<i<<"\'s maximum: "<<*single_max[i]<<'\n';
            int tmp_max=input[begin];
            for(std::size_t j=begin;j<end;j++) {
                tmp_max=std::max(tmp_max,input[j]);
            }
            if(tmp_max!=*single_max[i]) {
                pass_flag=false;
            }
        } else {
            std::cout<<"thread i's maximum is null\n";
        }
        begin=end;
    }
    int expected_max=input[0];
    for(std::size_t i=0;i<input.size();i++) {
        expected_max=std::max(expected_max,input[i]);
    }
    std::cout<<"CAS maximum result: "<<max_value.load()<<'\n';
    if(expected_max!=max_value.load()) {
        pass_flag=false;
    }
    if(pass_flag) {
        std::cout<<"success\n";
        return true;
    } else {
        std::cout<<"fail\n";
        return false;
    }
}

int main() {
    std::vector<int>input{1,2,3,4,5,6,7,8,9,10};
    if(!test(input,2)) {
        return 1;
    }
    input={-1};
    if(!test(input,2)) {
        return 1;
    }
    input={-1,-5,-24,-135,-124,-24352};
    if(!test(input,2)) {
        return 1;
    }
    input={-1,-5,-24,-135,-124,-24352};
    if(!test(input,10)) {
        return 1;
    }
    input={1,1,1,1,1};
    if(!test(input,10)) {
        return 1;
    }
    input={1,2,3,4,5};
    if(!test(input,10)) {
        return 1;
    }
    input={6,5,4,3,2,1};
    if(!test(input,10)) {
        return 1;
    }
    return 0;
}