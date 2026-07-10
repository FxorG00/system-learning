/*
1. 写 Buffer
2. 析构释放 char[]
3. 拷贝构造深拷贝
4. 赋值运算符用 copy-and-swap
5. 测 a = b
6. 测 a = a
7. 能解释旧资源什么时候释放
*/
#include <iostream>
#include <cctype>
#include <cstring>
#include <stdexcept>
class Buffer {
public:
    Buffer(std::size_t size):size_(size),data_(new char[size]{}) {

    }
    ~Buffer() {
        delete[] data_;
    }
    Buffer(const Buffer& other):size_(other.size_),data_(new char[other.size_]{}) {
        for(std::size_t i=0;i<size_;i++) data_[i]=other.data_[i];
    }
    void swap(Buffer& other) noexcept {
        std::swap(size_,other.size_);
        std::swap(data_,other.data_);
    }
    Buffer& operator=(Buffer temp) {
        swap(temp);
        return *this;
    }
    std::size_t size() const noexcept {
        return size_;
    }
private:
    std::size_t size_;
    char* data_;
};

void work() {
    Buffer a(10);
    Buffer b(2);
    a=a;
    a=b;
    std::cout<<a.size()<<" "<<b.size()<<'\n';
}

int main() {
    work();
    return 0;
}