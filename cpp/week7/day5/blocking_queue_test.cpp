#include "blocking_queue.hpp"
#include <vector>
#include <map>
#include <stdexcept>
#include <chrono>
#include <thread>

const int sentinel=998244353;
void consumer_work(BlockingQueue<int>& my_blocking_queue,std::vector<std::vector<int> >& consumer_vector,std::size_t index) {
    while(1) {
        std::optional<int> value=my_blocking_queue.pop();
        if(value) {
            consumer_vector[index].push_back(*value);
        } else {
            // 此时无值，结束 consumer_work
            return ;
        }   
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void producer_work(BlockingQueue<int>& my_blocking_queue,int begin,int end) {
    for(int i=begin;i<end;i++) {
        if(!my_blocking_queue.push(i)) {
            return ;
        }
    }
}

bool test(std::size_t capacity,std::size_t producer_count,std::size_t consumer_count,int N) {
    if(producer_count==0) {
        return false;
    }
    BlockingQueue<int> my_blocking_queue(capacity);
    std::vector<std::vector<int> >consumer_vector(consumer_count);
    std::vector<std::thread>producers,consumers;
    producers.reserve(producer_count);
    consumers.reserve(consumer_count);
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers.emplace_back(consumer_work,std::ref(my_blocking_queue),std::ref(consumer_vector),i);
    }
    int begin=0,base=N/producer_count,remainder=N%producer_count;
    for(std::size_t i=0;i<producer_count;i++) {
        int end=begin+base+(remainder>0);
        if(remainder>0) {
            --remainder;
        }
        producers.emplace_back(producer_work,std::ref(my_blocking_queue),begin,end);
        begin=end;
    }
    // 先让 producers 均完成 push
    for(std::size_t i=0;i<producer_count;i++) {
        producers[i].join();
    }
    // 再 close 后让 consumer 也结束
    my_blocking_queue.close();
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers[i].join();
    }
    // test repeated close
    my_blocking_queue.close();
    my_blocking_queue.close();
    // std::cout<<"ok";
    std::map<int,int>mp;
    for(std::size_t i=0;i<consumer_count;i++) {
        // std::cout<<"consumer "<<i<<": \n";
        for(auto x:consumer_vector[i]) {
            mp[x]++;
            // std::cout<<x<<' ';
        }
        // std::cout<<'\n';
    }
    for(int i=0;i<=N-1;i++) {
        if(mp[i]!=1) {
            return false;
        }
    }
    std::cout<<"PASS\n";
    if(my_blocking_queue.push(10)) {
        std::cout<<"push after close successfully\n";
    } else {
        std::cout<<"push after close failed\n";
    }
    return true;
}

bool test_close_with_data(std::size_t capacity,std::size_t producer_count,std::size_t consumer_count,int N) {
    if(producer_count==0) {
        return false;
    }
    BlockingQueue<int> my_blocking_queue(capacity);
    std::vector<std::vector<int> >consumer_vector(consumer_count);
    std::vector<std::thread>producers,consumers;
    producers.reserve(producer_count);
    consumers.reserve(consumer_count);
    int begin=0,base=N/producer_count,remainder=N%producer_count;
    for(std::size_t i=0;i<producer_count;i++) {
        int end=begin+base+(remainder>0);
        if(remainder>0) {
            --remainder;
        }
        producers.emplace_back(producer_work,std::ref(my_blocking_queue),begin,end);
        begin=end;
    }
    my_blocking_queue.close();
    for(std::size_t i=0;i<producer_count;i++) {
        producers[i].join();
    }
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers.emplace_back(consumer_work,std::ref(my_blocking_queue),std::ref(consumer_vector),i);
    }
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers[i].join();
    }
    std::cout<<"test close with data ok\n";
    return true;
}

bool test_multiple_blocked_consumers(std::size_t capacity,std::size_t producer_count,std::size_t consumer_count,int N) {
    if(producer_count==0) {
        return false;
    }
    BlockingQueue<int> my_blocking_queue(capacity);
    std::vector<std::vector<int> >consumer_vector(consumer_count);
    std::vector<std::thread>consumers;
    consumers.reserve(consumer_count);  
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers.emplace_back(consumer_work,std::ref(my_blocking_queue),std::ref(consumer_vector),i);
    }
    my_blocking_queue.close();
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers[i].join();
    }
    std::cout<<"test_multiple_blocked_consumers ok\n";
    return true;
}

bool test_multiple_blocked_producers(std::size_t capacity,std::size_t producer_count,std::size_t consumer_count,int N) {
    if(producer_count==0) {
        return false;
    }
    BlockingQueue<int> my_blocking_queue(capacity);
    std::vector<std::thread>producers;
    producers.reserve(producer_count);
    int begin=0,base=N/producer_count,remainder=N%producer_count;
    for(std::size_t i=0;i<producer_count;i++) {
        int end=begin+base+(remainder>0);
        if(remainder>0) {
            --remainder;
        }
        producers.emplace_back(producer_work,std::ref(my_blocking_queue),begin,end);
        begin=end;
    }
    my_blocking_queue.close();
    for(std::size_t i=0;i<producer_count;i++) {
        producers[i].join();
    }
    std::cout<<"test_multiple_blocked_producers ok\n";
    return true;
}

int main() {
    if(!test(101,4,3,100)) {
        return 1;
    }
    if(!test(1,1,1,100)) {
        return 1;
    }
    if(!test(1,2,2,100)) {
        return 1;
    }
    if(!test(1,4,1,500)) {
        return 1;
    }
    if(!test_close_with_data(1,4,4,100)) {
        return 1;
    }
    if(!test_multiple_blocked_consumers(1,4,4,100)) {
        return 1;
    }
    if(!test_multiple_blocked_producers(1,4,4,100)) {
        return 1;
    }
    try {
        test(0,1,1,1);
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
    }
    return 0;
}