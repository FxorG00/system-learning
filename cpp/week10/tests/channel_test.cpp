#include "channel.hpp"
#include <iostream>
#include <cassert>
#include <gtest/gtest.h>

TEST(ChannelTest,InitialState) {
    Channel channel(5);
    EXPECT_EQ(channel.fd(),5);
    EXPECT_EQ(channel.interest_events(),0);
    EXPECT_EQ(channel.ready_events(),0);
}

TEST(ChannelTest,InterestAndReadyAreIndependent) {
    Channel channel(5);
    channel.set_interest_events(EPOLLIN|EPOLLOUT);
    EXPECT_EQ(channel.interest_events(),EPOLLIN|EPOLLOUT);
    channel.set_ready_events(EPOLLIN);
    EXPECT_EQ(channel.ready_events(),EPOLLIN);
    channel.set_interest_events(EPOLLOUT);
    EXPECT_EQ(channel.ready_events(),EPOLLIN);
    EXPECT_EQ(channel.interest_events(),EPOLLOUT);
}
// 对应 callback 为空时跳过，不抛 std::bad_function_call
TEST(ChannelTest,MissingCallbackIsSkipped) {
    Channel channel(5);
    channel.set_ready_events(EPOLLIN);
    channel.handle_event();
}

TEST(ChannelTest,CombinedReadWriteDispatchesBoth) {
    Channel channel(5);
    channel.set_ready_events(EPOLLIN|EPOLLOUT);
    int value=5;
    channel.set_read_callback([&value]{std::cout<<"this is read call_back, and value: "<<value<<'\n';});
    channel.set_write_callback([&value]{std::cout<<"this is write call_back\n"; ++value;});
    channel.handle_event();
}