#include "event_loop.hpp"

#include <cerrno>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

class SocketPair {
public:
    SocketPair() {
        if (::socketpair(AF_UNIX,
                         SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                         0,
                         fds_) == -1) {
            throw std::system_error(
                errno, std::generic_category(), "socketpair");
        }
    }

    ~SocketPair() {
        ::close(fds_[0]);
        ::close(fds_[1]);
    }

    SocketPair(const SocketPair&) = delete;
    SocketPair& operator=(const SocketPair&) = delete;
    SocketPair(SocketPair&&) = delete;
    SocketPair& operator=(SocketPair&&) = delete;

    int sender() const noexcept { return fds_[0]; }
    int receiver() const noexcept { return fds_[1]; }

private:
    int fds_[2] = {-1, -1};
};

} // namespace

int main() {
    try {
        SocketPair sockets;
        EventLoop loop;
        Channel channel(sockets.receiver());

        int read_calls = 0;
        int write_calls = 0;
        char observed = '\0';

        channel.set_read_callback([&] {
            const ssize_t count =
                ::recv(sockets.receiver(), &observed, 1, 0);
            require(count == 1, "read callback did not receive one byte");
            ++read_calls;
        });
        channel.set_write_callback([&] { ++write_calls; });

        channel.set_interest_events(EPOLLIN);
        loop.add_channel(channel);

        require(loop.poll_once(20) == 0,
                "no-data poll should time out with zero records");

        const int sender_fd = sockets.sender();
        ssize_t send_result = -1;
        std::thread delayed_sender([sender_fd, &send_result] {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            send_result = ::send(sender_fd, "A", 1, MSG_NOSIGNAL);
        });

        int read_records = 0;
        try {
            read_records = loop.poll_once(-1);
        } catch (...) {
            delayed_sender.join();
            throw;
        }
        delayed_sender.join();

        require(send_result == 1, "delayed sender failed to send A");
        require(read_records == 1,
                "infinite wait poll should return one read record");
        require(read_calls == 1 && observed == 'A',
                "read readiness was not dispatched exactly once");

        channel.set_interest_events(EPOLLOUT);
        loop.update_channel(channel);
        const int write_records = loop.poll_once(20);
        require(write_records == 1,
                "one registered fd should produce one write record");
        require(write_calls == 1,
                "one write-ready record should be dispatched exactly once");
        require(read_calls == 1,
                "old read interest should not dispatch after MOD");

        channel.set_interest_events(EPOLLIN);
        loop.update_channel(channel);
        require(loop.poll_once(20) == 0,
                "MOD back to read should remove write readiness");
        require(write_calls == 1,
                "write callback should not repeat after MOD back to read");

        loop.remove_channel(channel);
        require(::send(sockets.sender(), "B", 1, MSG_NOSIGNAL) == 1,
                "failed to send B");
        require(loop.poll_once(20) == 0,
                "removed Channel should no longer produce records");
        require(read_calls == 1 && write_calls == 1,
                "callback ran after Channel removal");

        std::cout << "EVENT_LOOP_PROBE_PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "EVENT_LOOP_PROBE_FAIL: " << error.what() << '\n';
        return 1;
    }
}
