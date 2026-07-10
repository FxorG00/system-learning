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

    Buffer& operator =(Buffer&& other) noexcept {
        // 从 other 那里接管资源
        // move assignment 也是 assignment，要考虑 self assignment
        std::cout<<"move assignment size="<<other.size_<<std::endl;
        if(this==&other) {
            return *this;
        }
        delete[] data_;
        data_=other.data_;
        size_=other.size_;
        other.data_=nullptr;
        other.size_=0;
        // std::cout<<"other.size_= "<<other.size_<<std::endl;
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

int main() {
    Buffer a(5);
    Buffer b(10);
    std::cout<<a.size()<<" "<<b.size()<<std::endl;
    b=std::move(a);
    std::cout<<a.size()<<" "<<b.size()<<std::endl;
    return 0;
}