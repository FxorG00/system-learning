#include <iostream>
#include <cstddef>

class IntBuffer {
public:
    explicit IntBuffer(std::size_t size)
        : size_(size), data_(new int[size]) {
        std::cout << "construct, data = "
                  << static_cast<void*>(data_) << std::endl;

        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = 0;
        }
    }

    IntBuffer(const IntBuffer& other)
        : size_(other.size_), data_(new int[other.size_]) {
        std::cout << "copy construct, data = "
                  << static_cast<void*>(data_) << std::endl;

        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    IntBuffer& operator=(const IntBuffer& other) {
        std::cout << "copy assignment" << std::endl;

        if (this == &other) {
            std::cout << "self assignment, do nothing" << std::endl;
            return *this;
        }

        delete[] data_;

        size_ = other.size_;
        data_ = new int[size_];

        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }

        return *this;
    }

    ~IntBuffer() {
        std::cout << "destruct, data = "
                  << static_cast<void*>(data_) << std::endl;
        delete[] data_;
    }

    void set(std::size_t index, int value) {
        if (index < size_) {
            data_[index] = value;
        }
    }

    int get(std::size_t index) const {
        if (index < size_) {
            return data_[index];
        }
        return 0;
    }

private:
    std::size_t size_;
    int* data_;
};

int main() {
    IntBuffer a(3);
    a.set(0, 100);

    IntBuffer b(2);
    b.set(0, 999);

    b = a;  // 调用拷贝赋值运算符

    std::cout << "a[0] = " << a.get(0) << std::endl;
    std::cout << "b[0] = " << b.get(0) << std::endl;

    a = a;  // 自赋值测试

    return 0;
}