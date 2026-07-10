/*
1. 写一个 BadBuffer
2. 模拟先 delete 旧资源，再发生异常
3. catch 后观察对象状态
4. note 里解释它为什么不够稳
*/
#include <iostream>
#include <cctype>
#include <cstring>
#include <stdexcept>
class Buffer {
public:
    Buffer(std::size_t size):size_(size),data_(new int[size]) {

    }
    ~Buffer() {
        delete[] data_;
    }
    Buffer(const Buffer& other):size_(other.size_),data_(new int[other.size_]) {
        for(std::size_t i=0;i<size_;i++) data_[i]=other.data_[i];
    }
    Buffer& operator=(const Buffer& other) {
        delete[] data_;
        data_ = nullptr;
        size_ = 0;
        throw std::runtime_error("wrong!!!!");

    }
private:
    std::size_t size_;
    int* data_;
};

void work() {
    Buffer a(10);
    Buffer b(2);
    a=b;
}

int main() {
    try {
        work();
    } catch (const std::exception& e) {
        std::cout<<e.what()<<"\n";
    }
    return 0;
}