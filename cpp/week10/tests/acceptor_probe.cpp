#include "acceptor.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstdio>
#include <iostream>
#include <utility>
#include <vector>

int main() {
    EventLoop loop;
    std::vector<UniqueFd> accepted_connections;
    int count=0;
    // Bind to loopback with port 0, so the kernel selects a free port.
    Acceptor acceptor(loop, 0);

    // This is the upper-layer callback: it becomes the fd owner.
    acceptor.set_new_connection_callback(
        [&accepted_connections,&count](UniqueFd connection) {
            accepted_connections.push_back(std::move(connection));
            ++count;
        }
    );

    acceptor.start();
    if(acceptor.port()==0) {
        std::cerr<<"invalid port\n";
        return 1;
    }
    std::cout<<"port: "<<acceptor.port()<<'\n';
    for(int i=0;i<3;i++) {
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
        char payload[]="A";
        ::send(client.get(),payload,1,0);
    }

    // This wait should dispatch the listener Channel exactly once.
    const int ready_records = loop.poll_once(1000);

    if (ready_records != 1 || accepted_connections.size() != 3) {
        std::cerr << "unexpected result: records="
                  << ready_records
                  << ", connections="
                  << accepted_connections.size() << '\n';
        return 1;
    }
    if(accepted_connections.size()!=3) {
        return 1;
    }
    for(auto& tmp:accepted_connections) {
        if(!tmp) {
            return 1;
        }
        const int status_flags = ::fcntl(tmp.get(), F_GETFL);
        const bool nonblocking =
            status_flags != -1 &&
            (status_flags & O_NONBLOCK) != 0;

        const int descriptor_flags = ::fcntl(tmp.get(), F_GETFD);
        const bool cloexec =
            descriptor_flags != -1 &&
            (descriptor_flags & FD_CLOEXEC) != 0;
        if(!nonblocking||!cloexec) {
            return 1;
        }
        char buffer[8];
        ssize_t n=::recv(tmp.get(),buffer,sizeof(buffer),0);
        if(n!=1) {
            return 1;
        }
        if(buffer[0]!='A') {
            return 1;
        }
    }
    std::cout << "ACCEPTOR_PASS\n";
    return 0;
}