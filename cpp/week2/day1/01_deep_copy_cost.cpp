#include <cstddef>
#include <iostream>

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

Buffer make_buffer(std::size_t size) {
    Buffer tmp(size);
    tmp.set(0, 'A');
    return tmp;
}

int main() {
    Buffer b = make_buffer(10);
    std::cout << b.get(0) << '\n';
    return 0;
}