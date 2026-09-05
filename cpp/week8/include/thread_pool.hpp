#pragma once

#include <cstddef>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <future>
#include "blocking_queue.hpp"
#include <vector>
#include <memory>
#include <type_traits>
#include <utility>

using Task = std::function<void()>;

class ThreadPool {
public:
    // constructor
    // 应该创建 workers，并且做好创建失败去 join 以及抛出原异常的准备
    // 初始化好 tasks
    // 让 workers 开始运行，不断接收任务并且处理
    ThreadPool(std::size_t worker_count, std::size_t queue_capacity):tasks(queue_capacity) {
        if(worker_count==0) {
            throw std::invalid_argument("invalid argument: worker_count=0\n");
        }
        workers.reserve(worker_count);
        try {
            for(std::size_t i=0;i<worker_count;i++) {
                workers.emplace_back(work,std::ref(tasks));
            }
        } catch(...) {
            tasks.close();
            for(std::thread& worker: workers) {
                if(worker.joinable()) {
                    worker.join();
                }
            }
            // 再把原 exception 抛出
            throw;
        }
    }
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void shutdown() {
        tasks.close();
        // 关闭了 tasks 这个 queue 了之后，workers 会 drain 剩余的 tasks
        // 于是我们需要等待每个 worker 都完成了任务
        for(std::size_t i=0;i<workers.size();i++) {
            if(workers[i].joinable()) {
                workers[i].join();
            }
        }
    }
    // return std::future<std::invoke_result_t<F, Args...>>
    // function 是一个函数
    // 然后 args 是传的参数
    // 我需要用 packaged_task 去 wrapper 成 no argument callable

    template <class F, class... Args>
    auto submit(F&& function, Args&&... args) {
        using return_type=std::invoke_result_t<F, Args...>;
        auto later=std::bind(std::forward<F>(function),std::forward<Args>(args)...);
        std::packaged_task<return_type()> task(std::move(later));
        // 因为对 packaged_task 来说，我并不需要知道其内部 callable 返回值
        // 而且 packaged_task() 是没有返回值的
        // 其返回值会在 task 执行结束后写入 shared_state
        // consumer 可通过 future 去 get
        std::future<return_type> future=task.get_future();
        // 搞出来 shared_ptr 指向这个 task
        // 这样后续 capture shared_ptr 的 lambda 就可以 copy 了
        auto shared_ptr=std::make_shared<std::packaged_task<return_type()> >(std::move(task));
        Task task_element=[shared_ptr]() {
            (*shared_ptr)();
        };
        if(tasks.push(std::move(task_element))) {
            return future;
        } else {
            throw std::runtime_error("submit after thread pool shutdown\n");
        }
    }
    ~ThreadPool() {
        // 手动调用一次 shutdown，防止 caller 未调用
        shutdown();
    }
private:
    // task 执行时抛异常不能让其影响到外面。
    // 所以要用 try catch 去捕获异常
    // 如果 task 抛异常了，认为该 task 失败，failed_count++
    // 否则认为该 task 成功，executed_count++
    static void work(BlockingQueue<Task>& tasks) {
        while(1) {
            std::optional<Task> value=tasks.pop();
            if(value) {
                // 如果调用 packaged_task 发生了 exception
                // 不会向外抛，会把 exception 写入 shared state
                (*value)();
            } else {
                return ;
            }
        }
    }    
    BlockingQueue<Task>tasks;
    std::vector<std::thread>workers;
};