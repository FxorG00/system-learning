#include <gtest/gtest.h>
#include "async_logger.hpp"
#include "thread_pool.hpp"
#include <sstream>
#include <map>
#include <atomic>
// normal test
// 向 logger 写入几个 string
// 然后去看看对应的 file 是不是这几个string
TEST(AsyncLoggerTest,NormalStringTest) {
    AsyncLogger logger("logger_api_demo.txt",1);
    std::string str[5]={"hello","hi","what","pretty","queue"};
    for(std::size_t i=0;i<5;i++) {
        ASSERT_TRUE(logger.log(str[i]));
    }
    ASSERT_EQ(logger.shutdown(),true);
    ASSERT_FALSE(logger.log("late"));
    std::ifstream input("logger_api_demo.txt");
    std::string line;
    int nw=0;
    while(std::getline(input,line)) {
        ASSERT_FALSE(nw==5);
        ASSERT_EQ(line,str[nw]);
        ++nw;
    }
    ASSERT_EQ(nw, 5);
}

TEST(AsyncLoggerTest,RejectsZeroCapacity) {
    EXPECT_THROW(AsyncLogger("logger_api_demo.txt",0),std::invalid_argument);
}

TEST(AsyncLoggerTest,RejectsOutputOpenFailure) {
    EXPECT_THROW(AsyncLogger("missing_parent_directory/async.log",1),std::runtime_error);
}

// 测试并发 log 情况也正确
// 并发写入若干 log
// 每个 log 标识清楚 producer ID & log ID
TEST(AsyncLoggerTest,AcceptsRecordsFromMultipleProducersExactlyOnce) {
    AsyncLogger logger("logger_api_demo.txt",5);
    ThreadPool pool(5,5);
    for(std::size_t i=0;i<10;i++) {
        pool.submit([i,&logger](){
            for(std::size_t j=0;j<10;j++) {
                std::ostringstream builder;
                builder << "Producer: " << i << " log: "<<j;
                std::string result = builder.str();
                logger.log(result);
            }
        });
    }
    pool.shutdown();
    // 先让 pool.shutdown，这样我提交的所有 task 也就是所有 log 都能写入到 pool 里
    // 保证每个 task 都被执行成功
    // 到这里的时候所有 log 已经都进入到 logger，那么我可以关闭 logger 了
    ASSERT_TRUE(logger.shutdown());
    // 接下来 check 每个 message 都出现了
    std::ifstream input("logger_api_demo.txt");
    std::string line;
    std::map<std::string,std::size_t>mp;
    while(std::getline(input,line)) {
        mp[line]++;
    }
    ASSERT_EQ(mp.size(),100);
    for(std::size_t i=0;i<10;i++) {
        for(std::size_t j=0;j<10;j++) {
            std::ostringstream builder;
            builder << "Producer: " << i << " log: "<<j;
            std::string result = builder.str();
            ASSERT_EQ(mp[result],1);
        }
    }
}

TEST(AsyncLoggerTest,RejectsLogAfterShutdownAndAllowsRepeatedShutdown) {
    AsyncLogger logger("logger_api_demo.txt",2);
    ASSERT_TRUE(logger.shutdown());
    ASSERT_TRUE(logger.shutdown());
    ASSERT_FALSE(logger.log("hello"));
}

// log 100 次，然后并发 shutdown，就不等待每一条 log 都写入
// 验证 100=被 logger 接受的数量+失败的数量，即 return true/false
// 以及 被接受的数量 = 实际文件行数
TEST(AsyncLoggerTest,AccountingTest) {
    AsyncLogger logger("logger_api_demo.txt",5);
    ThreadPool pool(5,5);
    std::atomic<std::size_t>accepted_count(0),failed_count(0);
    bool vis[10][10]{};
    for(std::size_t i=0;i<10;i++) {
        pool.submit([i,&logger,&accepted_count,&failed_count,&vis](){
            for(std::size_t j=0;j<10;j++) {
                std::ostringstream builder;
                builder << "Producer: " << i << " log: "<<j;
                std::string result = builder.str();
                if(logger.log(result)) {
                    accepted_count.fetch_add(1);
                    vis[i][j]=1;
                } else {
                    failed_count.fetch_add(1);
                    vis[i][j]=0;
                }
            }
        });
    }
    // 到这里的时候，所有 submit 都是成功的，但是不是每条 log 都是成功的
    // 先关闭 logger
    // 所有会让一些 log 被 logger 拒绝
    ASSERT_TRUE(logger.shutdown());
    // 再关闭 thread_pool，让每条 submit 的 task 都被执行完成，保证每条 log 都调用
    pool.shutdown();
    ASSERT_EQ(accepted_count.load()+failed_count.load(),100);
    // 接下来 check 每个 message 都出现了
    std::ifstream input("logger_api_demo.txt");
    std::string line;
    std::map<std::string,std::size_t>mp;
    while(std::getline(input,line)) {
        mp[line]++;
    }
    ASSERT_EQ(mp.size(),accepted_count.load());
    for(std::size_t i=0;i<10;i++) {
        for(std::size_t j=0;j<10;j++) {
            std::ostringstream builder;
            builder << "Producer: " << i << " log: "<<j;
            std::string result = builder.str();
            if(vis[i][j]) {
                ASSERT_EQ(mp[result],1);
            }
        }
    }
}

// 提交 log 是可以的，因为只是把 record 送到 queue 里面
// 但是当 writer 在写这条 record 的时候就会失败，把 write_failed 改成 true
// shutdown 的时候，writer 需要去 flush 也会失败
TEST(AsyncLoggerTest,DevFullTest) {
    AsyncLogger logger("/dev/full", 8);

    // 此时 queue open、为空且 capacity=8，因此这次 log() 必须返回 true。
    const bool accepted = logger.log("runtime-failure");
    const bool shutdown_ok = logger.shutdown();
    ASSERT_TRUE(accepted);
    ASSERT_FALSE(shutdown_ok);
}