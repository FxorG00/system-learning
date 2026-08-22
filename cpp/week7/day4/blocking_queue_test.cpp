#include "blocking_queue.hpp"
#include <vector>
#include <map>
#include <stdexcept>
const int sentinel=998244353;
void consumer_work(BlockingQueue<int>& my_blocking_queue,std::vector<std::vector<int> >& consumer_vector,std::size_t index) {
    while(1) {
        int value=my_blocking_queue.pop();
        if(value==sentinel) return ;
        consumer_vector[index].push_back(value);
    }
}

void producer_work(BlockingQueue<int>& my_blocking_queue,int begin,int end) {
    for(int i=begin;i<end;i++) {
        my_blocking_queue.push(i);
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
    for(std::size_t i=0;i<producer_count;i++) {
        producers[i].join();
    }
    for(std::size_t i=0;i<consumer_count;i++) {
        my_blocking_queue.push(sentinel);
    }
    for(std::size_t i=0;i<consumer_count;i++) {
        consumers[i].join();
    }
    
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
    return true;
}

int main() {
    if(!test(101,4,3,100)) {
        return 1;
    }
    if(!test(1,1,1,100)) {
        return 1;
    }
    if(!test(1,2,2,10000)) {
        return 1;
    }
    if(!test(3,4,3,10000)) {
        return 1;
    }
    try {
        test(0,1,1,1);
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';
    }
    return 0;
}