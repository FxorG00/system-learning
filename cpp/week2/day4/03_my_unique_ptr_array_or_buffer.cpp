// unique_ptr no copy, only move
#include <cstddef>
#include <iostream>
#include <utility>
#include <memory>

class Buffer {
public:
    explicit Buffer(std::size_t size): size_(size),data_(std::make_unique<char[]>(size)) {
        std::cout<<"this is constructor\n";
    }

    // move constructor
    // 从别人接管资源，并让别人的指针置空
    // 但是我们不需要手动 new,delete了！
    // 省下了，让 other.data_ 置空的过程
    Buffer(Buffer&& other)noexcept: size_(other.size_),data_(std::move(other.data_)) {
        std::cout<<"this is move constructor"<<std::endl;
        other.size_=0;
    }

    Buffer& operator =(Buffer&& other) noexcept {
        std::cout<<"this is move assignment"<<std::endl;
        if(this==&other) {
            return *this;
        }
        size_=other.size_;
        other.size_=0;
        data_=std::move(other.data_);
        return *this;
    }

    const char* data() const {
        return data_.get();
    }

    std::size_t size() const {
        return size_;
    }
private:
    std::size_t size_;
    std::unique_ptr<char[]> data_;
};

int main() {
    Buffer a(5);
    Buffer b=std::move(a);
    std::cout<<a.size()<<" "<<b.size()<<std::endl;
    return 0;
}