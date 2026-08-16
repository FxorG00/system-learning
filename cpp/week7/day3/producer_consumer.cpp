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
std::size_t capacity;
int N;
std::queue<int>q;
std::mutex queue_mutex;
std::condition_variable not_empty_cv,not_full_cv;
std::size_t produced_count,consumed_count;
std::vector<int>consumed_vector;

bool not_full_predicate() {
    return q.size()<capacity;
}

bool not_empty_predicate() {
    return !q.empty();
}

void producer_work() {
    for(int i=0;i<=N-1;i++) {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // 要等待 queue not full
        not_full_cv.wait(lock,not_full_predicate);
        q.push(i);
        ++produced_count;
        lock.unlock();
        not_empty_cv.notify_one();
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumer_work() {
    for(int i=0;i<=N-1;i++) {
        // 明确 pop N 次
        std::unique_lock<std::mutex> lock(queue_mutex);
        not_empty_cv.wait(lock,not_empty_predicate);
        // 现在持有 queue_mutex 且 queue not empty
        int value=q.front(); q.pop();
        ++consumed_count;
        lock.unlock();
        not_full_cv.notify_one();
        consumed_vector.push_back(value);
    }
}

int main() {
    std::cin>>capacity>>N;
    std::thread consumer(consumer_work),producer(producer_work);
    consumer.join();
    producer.join();
    std::cout<<"consumed vector:\n";
    for(auto x:consumed_vector) {
        std::cout<<x<<' ';
    }
    std::cout<<'\n';
    std::cout<<"produced count: "<<produced_count<<'\n';
    std::cout<<"consumed count: "<<consumed_count<<'\n';
    std::sort(consumed_vector.begin(),consumed_vector.end());
    bool flag=true;
    // check 每个 ID 是否恰好出现一次
    for(std::size_t i=1;i<consumed_vector.size();i++) {
        if(consumed_vector[i]==consumed_vector[i-1]) {
            flag=false;
        }
    }
    if(flag) {
        std::cout<<"PASS\n";
    } else {
        std::cout<<"FAIL\n";
    }
    return 0;
}