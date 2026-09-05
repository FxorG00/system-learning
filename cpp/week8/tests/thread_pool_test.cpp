#include <gtest/gtest.h>
#include "thread_pool.hpp"

TEST(ThreadPoolTest,ConstructWithZeroWorkerCount) {
    EXPECT_THROW(ThreadPool(0,10),std::invalid_argument);
    EXPECT_THROW(ThreadPool(10,0),std::invalid_argument);
}

TEST(ThreadPoolTest,SubmitWithReturnIntValue) {
    ThreadPool pool(10,10);
    auto future=pool.submit([](){return 101;});
    int value=10;
    auto future2=pool.submit([&value](){value=100;});
    pool.shutdown();
    EXPECT_EQ(future.get(),101);
    future2.get();
    EXPECT_EQ(value,100);
}

TEST(ThreadPoolTest,SubmitWithReturnVoid) {
    ThreadPool pool(10,10);
    int value=10;
    auto future=pool.submit([&value](){value=100;});
    pool.shutdown();
    future.get();
    EXPECT_EQ(value,100);
}

TEST(ThreadPoolTest,SubmitWithThrowExceptionCallable) {
    ThreadPool pool(10,10);
    auto future=pool.submit([](){throw std::runtime_error("this is an exception");});
    EXPECT_THROW(future.get(),std::runtime_error);
    // 要验证 pool 仍然 survival
    // 再 submit 一个
    auto future2=pool.submit([](){return 2;});
    EXPECT_EQ(future2.get(),2);
}

TEST(ThreadPoolTest,SubmitAfterShutdown) {
    ThreadPool pool(10,10);
    pool.shutdown();
    EXPECT_THROW(pool.submit([](){return 10;}),std::runtime_error);
}

TEST(ThreadPoolTest,AllTestBecomeCompleted) {
    const std::size_t task_count=10;
    ThreadPool pool(5,5);
    std::vector<std::future<void> >futures;
    std::vector<std::size_t> counters(task_count);
    for(std::size_t i=0;i<task_count;i++) {
        auto future=pool.submit([i,&counters](){++counters[i];});
        futures.push_back(std::move(future));
    }
    pool.shutdown();
    for(std::size_t i=0;i<task_count;i++) {
        futures[i].get();
    }
    for(std::size_t i=0;i<task_count;i++) {
        EXPECT_EQ(counters[i],1);
    }
}

TEST(ThreadPoolTest,StillPendingTasksWhenShutdown) {
    ThreadPool pool(1,1);
    std::mutex mutex;
    std::condition_variable cv;
    bool gate=false;
    bool hit_flag[3];
    memset(hit_flag,false,sizeof(hit_flag));
    pool.submit([&mutex,&cv,&gate,&hit_flag](){
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock,[&gate](){return gate;});
        // 此时拿到 lock 且 gate = true
        // 则 task A 其实已经完成了
        hit_flag[0]=true;
    });
    // 提交 task B
    pool.submit([&](){hit_flag[1]=true;});
    // helper thread 调用 shutdown
    // 记得 join 这个 thread，否则这个 thread 会离开 scope 然后自动析构，调用 terminate
    std::thread helper([&pool](){
        pool.shutdown();
    });
    // 再提交 task C，但是此时 queue 已经 close，会被拒绝
    EXPECT_THROW(pool.submit([&](){hit_flag[2]=true;}),std::runtime_error);
    // 此时获取锁，并且将 gate=true
    // 即让阻塞的 task A 接着往下运行
    {
        std::unique_lock<std::mutex> u_lock(mutex);
        gate=true;
        u_lock.unlock();
        // 记得 cv.notify_one();
        cv.notify_all();
    }
    helper.join();
    // 验证结束
    // check hit_flag
    EXPECT_EQ(hit_flag[0],true);
    EXPECT_EQ(hit_flag[1],true);
    EXPECT_EQ(hit_flag[2],false);
}

// 我们提交 empty function 的话，会在 worker 执行 packaged_task 的时候
// 出现异常，那么会把异常写到 future 里
// 我们用 future.get() 看看是否抛了异常即可
TEST(ThreadPoolTest,SubmitWithEmptyFunction) {
    ThreadPool pool(1,1);
    std::function<void()> empty;
    auto future=pool.submit(empty);
    EXPECT_THROW(future.get(),std::bad_function_call);
    // 要验证 pool 仍然 survival
    // 再 submit 一个
    auto future2=pool.submit([](){return 2;});
    EXPECT_EQ(future2.get(),2);
}

// 我需要测试 multiple concurrent submitters 是正确的
// 比如 submitter 0 提交 [0,K) 这个范围的 task
// submitter 1 提交 [K,2K) 这个范围的 task
// 然后我最后汇总，要求每个 task 被执行 exactly one
// task_i 就 return i
// 通过 future 去 get return then check the return value set
void func(ThreadPool &pool,std::vector<std::future<std::size_t> >& futures,std::mutex& futures_mutex,std::size_t begin,std::size_t end) {
    for(std::size_t i=begin;i<end;i++) {
        auto future=pool.submit([i](){return i;});
        std::lock_guard<std::mutex> guard(futures_mutex);
        futures.push_back(std::move(future));
    }
}
TEST(ThreadPoolTest,MultipleConcurrentSubmitters) {
    ThreadPool pool(5,5);
    const std::size_t K=5,submitter_count=5,task_count=K*submitter_count;
    std::vector<std::thread>submitters;
    std::vector<std::future<std::size_t> >futures;
    std::mutex futures_mutex; // 用来保护 futures
    submitters.reserve(submitter_count);
    for(std::size_t i=0;i<submitter_count;i++) {
        submitters.emplace_back(func,std::ref(pool),std::ref(futures),std::ref(futures_mutex),i*K,(i+1)*K);
    }
    for(std::size_t i=0;i<submitter_count;i++) {
        submitters[i].join();
    }
    pool.shutdown();
    std::map<std::size_t,std::size_t>mp;
    for(auto& future:futures) {
        ++mp[future.get()];
    }
    for(std::size_t i=0;i<task_count;i++) {
        EXPECT_EQ(mp[i],1);
    }
}

// 验证 thread_pool destructor 会 drain all accepted tasks and join all workers
// 就是在 inner scope 创建 thread_pool，然后不主动 shutdown，等待离开 scope 调用 destructor
// 然后 outer scope 验证每个 future 都 ready 了，这样能证明每个 task 都被执行
TEST(ThreadPoolTest,CheckDestructorContract) {
    std::vector<std::future<std::size_t> >futures;
    const std::size_t K=5,submitter_count=5,task_count=K*submitter_count;
    {
        ThreadPool pool(5,5);
        std::vector<std::thread>submitters;
        std::mutex futures_mutex; // 用来保护 futures
        submitters.reserve(submitter_count);
        for(std::size_t i=0;i<submitter_count;i++) {
            submitters.emplace_back(func,std::ref(pool),std::ref(futures),std::ref(futures_mutex),i*K,(i+1)*K);
        }
        for(std::size_t i=0;i<submitter_count;i++) {
            submitters[i].join();
        }
    }
    std::map<std::size_t,std::size_t>mp;
    for(auto& future:futures) {
        ++mp[future.get()];
    }
    for(std::size_t i=0;i<task_count;i++) {
        EXPECT_EQ(mp[i],1);
    }
}