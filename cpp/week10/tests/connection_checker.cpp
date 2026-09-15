#include "connection.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <system_error>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

// A component-level checker for Connection. It uses a local stream socketpair,
// so Acceptor and a complete TCP server are not required.
namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void send_all(int fd, const char* data, std::size_t length) {
    // Feed exact test bytes into the peer side of the socketpair.
    std::size_t offset = 0;
    while (offset < length) {
        const ssize_t count =
            ::send(fd, data + offset, length - offset, MSG_NOSIGNAL);
        if (count > 0) {
            offset += static_cast<std::size_t>(count);
            continue;
        }
        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd event{fd, POLLOUT, 0};
            const int result = ::poll(&event, 1, 1000);
            require(result == 1 && (event.revents & POLLOUT) != 0,
                    "peer did not become writable");
            continue;
        }
        throw std::system_error(errno, std::generic_category(), "peer send");
    }
}

void drain_available(int fd, std::string& output) {
    // Drain only the bytes currently available on the non-blocking peer.
    char buffer[16384];
    for (;;) {
        const ssize_t count = ::recv(fd, buffer, sizeof(buffer), 0);
        if (count > 0) {
            output.append(buffer, static_cast<std::size_t>(count));
            continue;
        }
        if (count == -1 && errno == EINTR) {
            continue;
        }
        if (count == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        if (count == 0) {
            throw std::runtime_error("unexpected peer EOF");
        }
        throw std::system_error(errno, std::generic_category(), "peer recv");
    }
}

std::string receive_exact(int fd, std::size_t expected_size) {
    // Wait with a deadline instead of allowing a broken checker to hang.
    std::string received;
    received.reserve(expected_size);

    while (received.size() < expected_size) {
        drain_available(fd, received);
        if (received.size() == expected_size) {
            break;
        }

        pollfd event{fd, POLLIN, 0};
        const int result = ::poll(&event, 1, 1000);
        require(result == 1 && (event.revents & POLLIN) != 0,
                "timed out before exact response arrived");
    }

    require(received.size() == expected_size,
            "received more bytes than the expected response");
    return received;
}

} // namespace

int main() {
    try {
        int raw_fds[2] = {-1, -1};
        if (::socketpair(AF_UNIX,
                         SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                         0,
                         raw_fds) == -1) {
            throw std::system_error(
                errno, std::generic_category(), "socketpair");
        }

        UniqueFd peer(raw_fds[0]);
        UniqueFd connection_fd(raw_fds[1]);

        // A small kernel send buffer makes partial write/EAGAIN observable.
        const int small_send_buffer = 4096;
        if (::setsockopt(connection_fd.get(),
                         SOL_SOCKET,
                         SO_SNDBUF,
                         &small_send_buffer,
                         sizeof(small_send_buffer)) == -1) {
            throw std::system_error(
                errno, std::generic_category(), "setsockopt(SO_SNDBUF)");
        }

        std::cout << "CHECK 1: construct and start\n";
        EventLoop loop;
        Connection connection(loop, std::move(connection_fd));

        int message_calls = 0;
        int close_calls = 0;
        int closed_fd = -1;

        connection.set_message_callback(
            [&message_calls](Connection& current, Buffer& input) {
                ++message_calls;

                while (!input.empty()) {
                    const char* begin = input.peek();
                    const void* found =
                        std::memchr(begin, '\n', input.readable_bytes());
                    if (found == nullptr) {
                        return;
                    }

                    const char* newline = static_cast<const char*>(found);
                    const std::size_t line_length =
                        static_cast<std::size_t>(newline - begin) + 1;

                    current.send(begin, line_length);
                    input.retrieve(line_length);
                }
            });
        connection.set_close_callback(
            [&close_calls, &closed_fd](int fd) {
                ++close_calls;
                closed_fd = fd;
            });
        connection.start();
        require(connection.started(), "Connection did not enter started state");
        std::cout << "CHECK 1 PASS\n";

        std::cout << "CHECK 2: fragmented input and MessageCallback\n";
        send_all(peer.get(), "hel", 3);
        require(loop.poll_once(1000) == 1,
                "first input did not produce one ready record");
        require(message_calls == 1,
                "MessageCallback should run after the first read drain");
        require(connection.pending_input_bytes() == 3,
                "incomplete input suffix was not preserved");

        char unexpected = '\0';
        const ssize_t early_read = ::recv(peer.get(), &unexpected, 1, 0);
        require(early_read == -1 &&
                    (errno == EAGAIN || errno == EWOULDBLOCK),
                "incomplete line produced an unexpected response");

        send_all(peer.get(), "lo\nworld\n", 9);
        require(loop.poll_once(1000) == 1,
                "second input did not produce one ready record");
        require(message_calls == 2,
                "MessageCallback count does not match two read drains");
        require(connection.pending_input_bytes() == 0,
                "complete lines were not fully consumed");
        require(receive_exact(peer.get(), 12) == "hello\nworld\n",
                "fragmented/coalesced echo bytes do not match");
        std::cout << "CHECK 2 PASS\n";

        std::cout << "CHECK 3: pending output and EPOLLOUT resume\n";
        const std::string payload(2 * 1024 * 1024, 'x');
        connection.send(payload.data(), payload.size());
        require(connection.pending_output_bytes() > 0,
                "checker failed to establish pending output");

        std::string received;
        received.reserve(payload.size());
        int rounds = 0;
        while (received.size() < payload.size() && rounds < 10000) {
            drain_available(peer.get(), received);
            if (received.size() == payload.size()) {
                break;
            }

            if (connection.pending_output_bytes() > 0) {
                require(loop.poll_once(1000) > 0,
                        "pending output was not resumed by EPOLLOUT");
            }
            ++rounds;
        }
        std::cout<<received.length()<<" "<<payload.length()<<'\n';
        require(received == payload,
                "large output was lost, duplicated, or reordered");
        require(connection.pending_output_bytes() == 0,
                "output Buffer was not empty after complete delivery");
        std::cout << "CHECK 3 PASS\n";

        std::cout << "CHECK 4: peer EOF and idempotent close request\n";
        if (::shutdown(peer.get(), SHUT_WR) == -1) {
            throw std::system_error(
                errno, std::generic_category(), "shutdown(SHUT_WR)");
        }
        require(loop.poll_once(1000) > 0,
                "peer EOF did not produce a ready record");
        require(connection.peer_write_closed(),
                "Connection did not record peer EOF");
        require(close_calls == 1 && closed_fd == connection.fd(),
                "Connection did not submit exactly one close request");

        loop.poll_once(20);
        // std::cout<<close_calls<<'\n';
        require(close_calls == 1,
                "close request was submitted more than once");
        std::cout << "CHECK 4 PASS\n";

        std::cout << "CONNECTION_CHECKER_PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CONNECTION_CHECKER_FAIL: " << error.what() << '\n';
        return 1;
    }
}