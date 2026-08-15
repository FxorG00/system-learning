#include <thread>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <mutex>
class Ledger {
public:
    Ledger(const std::vector<std::int64_t>& initial_balances):balances_(initial_balances),success_request_count(0),failed_request_count(0) {

    }
    // 为 true 时代表 index invalid
    bool invalid(std::size_t index) {
        if(index>=balances_.size()) {
            return true;
        }
        return false;
    }
    bool transfer(std::size_t from,std::size_t to,std::int64_t amount) {
        // 拿 mutex（涉及到 shared state 的读写）
        // 再进行对 balances_[from/to] 的读取
        // check 完能够发生 transfer
        // 再进行修改 
        // 释放 mutex
        if(from==to||amount<=0||invalid(from)||invalid(to)) {
            std::lock_guard<std::mutex> guard(failed_request_count_mutex);
            ++failed_request_count;
            return false;
        }
        std::lock_guard<std::mutex> guard(mutex_);
        if(balances_[from]<amount) {
            std::lock_guard<std::mutex> count_guard(failed_request_count_mutex);
            ++failed_request_count;
            return false;
        }
        balances_[from]-=amount;
        balances_[to]+=amount;
        std::lock_guard<std::mutex> count_guard(success_request_count_mutex);
        ++success_request_count;
        return true;
    }
    std::vector<int64_t> snapshot() const {
        std::lock_guard<std::mutex> guard(mutex_);
        return balances_;
    }
    std::int64_t total() const {
        std::lock_guard<std::mutex> guard(mutex_);
        std::int64_t sum=0;
        for(auto value:balances_) {
            sum+=value;
        }
        return sum;
    }
    void print_count() const {
        std::lock_guard<std::mutex> failed_count_guard(failed_request_count_mutex);
        std::lock_guard<std::mutex> success_count_guard(success_request_count_mutex);
        std::cout<<"success count: "<<success_request_count<<'\n'<<"failed count: "<<failed_request_count<<'\n';
    }
private:
    std::vector<std::int64_t>balances_;
    mutable std::mutex mutex_,success_request_count_mutex,failed_request_count_mutex;
    std::size_t success_request_count,failed_request_count;
};

struct TransferRequest {
    std::size_t from,to;
    std::int64_t amount;
};

// 在 my_ledger 里执行 requests 中 [begin,end) 的 request
// 直接进行 transfer 即可，因为 transfer 内部的 mutex 已经保证了不会 data race
void work(Ledger& my_ledger,std::vector<TransferRequest>& requests,std::size_t begin,std::size_t end) {
    for(std::size_t i=begin;i<end;i++) {
        my_ledger.transfer(requests[i].from,requests[i].to,requests[i].amount);
    }
}

bool test(std::vector<std::int64_t>& initial_balances,std::vector<TransferRequest>& requests,std::size_t request_worker_count) {
    // range partition
    // 每个 worker 负责一个 range 的 requests
    for(auto x:initial_balances) {
        if(x<0) {
            return false;
        }
    }
    Ledger my_ledger(initial_balances);
    std::int64_t initial_total=my_ledger.total();
    std::size_t worker_count=std::min(requests.size(),request_worker_count);
    if(worker_count==0) {
        // 拒绝 woker_count=0 的情况
        return false;
    }
    std::vector<std::thread> workers;
    std::size_t base=requests.size()/worker_count,remainder=requests.size()%worker_count;
    std::size_t begin=0;
    for(std::size_t i=0;i<worker_count;i++) {
        std::size_t end=begin+base+(remainder>0);
        workers.emplace_back(work,std::ref(my_ledger),std::ref(requests),begin,end);
        if(remainder>0) {
            --remainder;
        }
        begin=end;
    }
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
    }
    std::int64_t final_total=my_ledger.total();
    const auto snapshot_balances=my_ledger.snapshot();
    if(final_total==initial_total) {
        std::cout<<"invariant PASS\n";
    } else {
        std::cerr<<"invariant FAILed\n";
        return false;
    }
    std::cout<<"initial total: "<<initial_total<<'\n';
    std::cout<<"final total: "<<final_total<<'\n';
    std::cout<<"final balances snapshot:\n";
    for(auto x:snapshot_balances) {
        std::cout<<x<<" ";
    }
    std::cout<<'\n';
    my_ledger.print_count();
    return true;
}

int main() {
    std::vector<std::int64_t> initial_balances{100, 200, 50};

    std::vector<TransferRequest> requests{
        {0, 1, 30},
        {1, 2, 80},
        {2, 0, 100},
    };

    std::size_t worker_count = 3;
    if(!test(initial_balances,requests,worker_count)) {
        return 1;
    }
    return 0;
}