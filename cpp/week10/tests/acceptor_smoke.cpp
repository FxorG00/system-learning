#include "acceptor.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>

int main() {
    EventLoop loop;
    std::vector<UniqueFd> accepted_connections;

    // Bind to loopback with port 0, so the kernel selects a free port.
    Acceptor acceptor(loop, 0);

    // This is the upper-layer callback: it becomes the fd owner.
    acceptor.set_new_connection_callback(
        [&accepted_connections](UniqueFd connection) {
            accepted_connections.push_back(std::move(connection));
        }
    );

    acceptor.start();

    // Create one blocking client only for this small smoke test.
    UniqueFd client(
        ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)
    );
    if (!client) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    server_address.sin_port = ::htons(acceptor.port());

    if (::connect(
            client.get(),
            reinterpret_cast<const sockaddr*>(&server_address),
            sizeof(server_address)) == -1) {
        std::perror("connect");
        return 1;
    }

    // This wait should dispatch the listener Channel exactly once.
    const int ready_records = loop.poll_once(1000);

    if (ready_records != 1 || accepted_connections.size() != 1) {
        std::cerr << "unexpected result: records="
                  << ready_records
                  << ", connections="
                  << accepted_connections.size() << '\n';
        return 1;
    }

    std::cout << "ACCEPTOR_SMOKE_PASS\n";
    return 0;
}