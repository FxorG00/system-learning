#include "channel.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/epoll.h>
#include <unistd.h>

TEST(ChannelCodexTest, ReadReadyDispatchesOnlyReadCallback) {
    Channel channel(5);
    int read_count = 0;
    int write_count = 0;
    int error_count = 0;

    channel.set_read_callback([&read_count] { ++read_count; });
    channel.set_write_callback([&write_count] { ++write_count; });
    channel.set_error_callback([&error_count] { ++error_count; });
    channel.set_ready_events(EPOLLIN);

    channel.handle_event();

    EXPECT_EQ(read_count, 1);
    EXPECT_EQ(write_count, 0);
    EXPECT_EQ(error_count, 0);
}

TEST(ChannelCodexTest, WriteReadyDispatchesOnlyWriteCallback) {
    Channel channel(5);
    int read_count = 0;
    int write_count = 0;

    channel.set_read_callback([&read_count] { ++read_count; });
    channel.set_write_callback([&write_count] { ++write_count; });
    channel.set_ready_events(EPOLLOUT);

    channel.handle_event();

    EXPECT_EQ(read_count, 0);
    EXPECT_EQ(write_count, 1);
}

TEST(ChannelCodexTest, CombinedReadWriteDispatchesEachExactlyOnce) {
    Channel channel(5);
    int read_count = 0;
    int write_count = 0;

    channel.set_read_callback([&read_count] { ++read_count; });
    channel.set_write_callback([&write_count] { ++write_count; });
    channel.set_ready_events(EPOLLIN | EPOLLOUT);

    channel.handle_event();

    EXPECT_EQ(read_count, 1);
    EXPECT_EQ(write_count, 1);
}

TEST(ChannelCodexTest, ErrorAndReadDispatchEachExactlyOnce) {
    Channel channel(5);
    int read_count = 0;
    int error_count = 0;

    channel.set_read_callback([&read_count] { ++read_count; });
    channel.set_error_callback([&error_count] { ++error_count; });
    channel.set_ready_events(EPOLLERR | EPOLLIN);

    channel.handle_event();

    EXPECT_EQ(read_count, 1);
    EXPECT_EQ(error_count, 1);
}

TEST(ChannelCodexTest, ZeroReadyDispatchesNothing) {
    Channel channel(5);
    int call_count = 0;

    channel.set_read_callback([&call_count] { ++call_count; });
    channel.set_write_callback([&call_count] { ++call_count; });
    channel.set_error_callback([&call_count] { ++call_count; });

    channel.handle_event();

    EXPECT_EQ(call_count, 0);
}

TEST(ChannelCodexTest, DispatchDoesNotClearMasks) {
    Channel channel(5);
    channel.set_interest_events(EPOLLIN | EPOLLOUT);
    channel.set_ready_events(EPOLLIN);

    channel.handle_event();

    EXPECT_EQ(channel.interest_events(), EPOLLIN | EPOLLOUT);
    EXPECT_EQ(channel.ready_events(), EPOLLIN);
}

TEST(ChannelCodexTest, DestroyingChannelDoesNotCloseFd) {
    int pipe_fds[2] = {-1, -1};
    ASSERT_EQ(::pipe(pipe_fds), 0);

    {
        Channel channel(pipe_fds[0]);
        EXPECT_EQ(channel.fd(), pipe_fds[0]);
    }

    EXPECT_NE(::fcntl(pipe_fds[0], F_GETFD), -1);
    EXPECT_EQ(::close(pipe_fds[0]), 0);
    EXPECT_EQ(::close(pipe_fds[1]), 0);
}
