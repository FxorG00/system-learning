#include "event_loop.hpp"
#include "acceptor.hpp"
#include "connection.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <utility>
int main() {
    EventLoop loop;
    Acceptor acceptor(loop, 9091, 128);
    std::unordered_map<int, std::unique_ptr<Connection>> connections;
    std::vector<int>pending_close;
    acceptor.set_new_connection_callback(
        [&](UniqueFd socket) {

            auto connection = std::make_unique<Connection>(loop, std::move(socket));

            const int fd = connection->fd();

            connection->set_message_callback([](Connection& connection,Buffer& input){
                // 找到 \n 就把 \n 及之前的都 send 到 connection
                // 其实我们找到最后一个 \n 即可
                std::size_t pos=0;
                if(input.readable_bytes()>0) {
                    for(std::size_t i=input.readable_bytes();i>0;i--) {
                        auto it=input.peek()+i-1;
                        if((*it)=='\n') {
                            pos=i;
                            break ;
                        }
                    }
                }
                if(pos!=0) {
                    connection.send(input.peek(),pos);
                    input.retrieve(pos);
                }
            });
            connection->set_close_callback([&pending_close](int fd) {pending_close.push_back(fd);});
            connection->start();
            connections.emplace(fd, std::move(connection));
    });

    acceptor.start();

    for (;;) {
        loop.poll_once(-1);
        // 在 dispatch 返回后处理本轮 pending cleanup。
        for (int fd : pending_close) {
            connections.erase(fd);
        }
        pending_close.clear();
    }
}