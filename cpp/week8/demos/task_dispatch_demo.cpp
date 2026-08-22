#include "../include/blocking_queue.hpp"
#include <atomic>
using Task = std::function<void()>;

void work(BlockingQueue<Task>& tasks,std::atomic<std::size_t>& executed_count) {
    while(1) {
        std::optional<Task> value=tasks.pop();
        if(value) {
            (*value)();
            executed_count.fetch_add(1);
        } else {
            return ;
        }
    }
}

void free_func(std::size_t i,std::vector<std::size_t>& results) {
    results[i]=2*i+5;
}

struct ComputeTask {
    std::size_t id;
    std::vector<std::size_t>* results;

    void operator()() const {
        (*results)[id] = static_cast<std::size_t>(2 * id + 5);
    }
};


bool test(std::size_t worker_count,std::size_t queue_capacity,std::size_t task_count) {
    if(queue_capacity==0) {
        std::cerr<<"invalid queue capacity: queue capacity must >0\n";
        return false;
    }
    BlockingQueue<Task> tasks(queue_capacity);
    std::vector<std::thread>workers;
    std::vector<std::size_t>results(task_count);
    workers.reserve(worker_count);
    std::atomic<std::size_t>executed_count(0);

    for(std::size_t i=0;i<worker_count;i++) {
        workers.emplace_back(work,std::ref(tasks),std::ref(executed_count));
    }
    for(std::size_t i=0;i<task_count;i++) {
        Task now_task;
        if(i%3==0) {
            now_task=[i,&results]() {
                results[i]=2*i+5;
            };
        } else if(i%3==1) {
            // 可以先用 lambda 捕获 i,&results
            // 再在 lambda 里调用 free function
            // 这样是不需要参数的，因为 lambda capture 的东西进入了闭包对象的内部。
            now_task=[i,&results]() {
                free_func(i,results);
            };
        } else {
            now_task=ComputeTask{i,&results};
        }
        // now_task 在这次提交后不再需要。
        // std::move 把表达式 std::move(now_task) 转成 xvalue，
        // 让 push 的参数可以通过移动构造得到，避免一次不必要的复制。
        const bool accepted = tasks.push(std::move(now_task));
        if(!accepted) {
            std::cerr<<"push task error\n";
            return false;
        }
    }
    tasks.close();
    for(std::size_t i=0;i<worker_count;i++) {
        workers[i].join();
    }
    bool success_flag=(executed_count==task_count);
    for(std::size_t i=0;i<task_count;i++) {
        if(results[i]!=2*i+5) {
            success_flag=false;
        }
    }
    std::cout<<"worker_count: "<<worker_count<<'\n';
    std::cout<<"queue_capacity: "<<queue_capacity<<'\n';
    std::cout<<"task_count: "<<task_count<<'\n';
    std::cout<<"executed_count: "<<executed_count<<'\n';
    if(success_flag) {
        std::cout<<"PASS\n";
        return true;
    } else {
        std::cout<<"FAIL\n";
        return false;
    }
}

int main() {
    if(!test(3,4,20)) {
        return 1;
    }
    if(!test(4,1,100)) {
        return 1;
    }
    if(!test(3,2,0)) {
        return 1;
    }
    return 0;
}