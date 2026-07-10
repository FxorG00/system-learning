#include <cstddef>
#include <iostream>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(new char[size]), size_(size) {
        std::cout << "construct size=" << size_ << '\n';
    }

    Buffer(const Buffer& other)
        : data_(new char[other.size_]), size_(other.size_) {
        std::cout << "copy construct size=" << size_ << '\n';
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    Buffer& operator=(const Buffer& other) {
        std::cout << "copy assignment size=" << other.size_ << '\n';
        if (this == &other) {
            return *this;
        }

        char* new_data = new char[other.size_];
        for (std::size_t i = 0; i < other.size_; ++i) {
            new_data[i] = other.data_[i];
        }

        delete[] data_;
        data_ = new_data;
        size_ = other.size_;
        return *this;
    }

    Buffer(Buffer&& other) noexcept
        : data_(other.data_), size_(other.size_) {
        std::cout << "move construct size=" << size_ << '\n';
        other.data_ = nullptr;
        other.size_ = 0;
    }

    ~Buffer() {
        std::cout << "destruct size=" << size_ << '\n';
        delete[] data_;
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
    char* data_;
    std::size_t size_;
};

int main() {
    Buffer a(5);
    a.set(0, 'A');

    Buffer b = a;
    std::cout << "b[0]=" << b.get(0) << '\n';

    Buffer c = std::move(a);
    std::cout << "c[0]=" << c.get(0) << '\n';
    std::cout << "a.size()=" << a.size() << '\n';

    return 0;
}