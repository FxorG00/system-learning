#include <string>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>
class ConnectionState {
public:
    ConnectionState():message_count_(0) {}
    // 追加 str 这部分 bytes
    void append(const std::string& data) {
        for(std::size_t i=0;i<data.size();i++) {
            input.push_back(data[i]);
            if(data[i]==delimiter) {
                // 这是分隔符
                for(auto c:input) {
                    output.push_back(c);
                }
                ++message_count_;
                input.clear();
            }
        }
    }

    const std::string& pending_input() const {
        return input;
    }
    const std::string& pending_output() const {
        return output;
    }
    std::size_t message_count() const {
        return message_count_;
    }
private:
    // 直接默认构造即可
    std::string input,output;
    std::size_t message_count_;
    static const char delimiter='\n';
};

int main() {
    ConnectionState a;
    a.append("hel");
    std::cout<<a.pending_input()<<'\n';
    std::cout<<a.message_count()<<'\n';
    std::cout<<a.pending_output()<<'\n';
    a.append("lo\nworld\npar");
    std::cout<<a.pending_input()<<'\n';
    std::cout<<a.message_count()<<'\n';
    std::cout<<a.pending_output()<<'\n';
    a.append("tial\n");
    std::cout<<a.pending_input()<<'\n';
    std::cout<<a.message_count()<<'\n';
    std::cout<<a.pending_output()<<'\n';
    if(a.pending_input()==""&&a.pending_output()=="hello\nworld\npartial\n"&&a.message_count()==3) {
        std::cout<<"PASS\n";
        return 0;
    } else {
        std::cout<<"FAIL\n";
        return 1;
    }
}