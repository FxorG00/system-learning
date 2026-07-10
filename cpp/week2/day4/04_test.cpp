#include <cstddef>
#include <iostream>
#include <memory>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(std::make_unique<char[]>(size)), size_(size) {
        std::cout << "Buffer construct size=" << size_ << '\n';
    }

    void set(std::size_t index, char value) {
        if (index >= size_) {
            return;
        }
        data_[index] = value;
    }

    char get(std::size_t index) const {
        if (index >= size_) {
            return ' ';
        }
        return data_[index];
    }

    std::size_t size() const {
        return size_;
    }

private:
    std::unique_ptr<char[]> data_;
    std::size_t size_;
};

int main() {
    Buffer a(5);
    Buffer c = std::move(a);  // 可以 move
    std::cout << a.size()<<" "<<c.size()<<std::endl;

    return 0;
}