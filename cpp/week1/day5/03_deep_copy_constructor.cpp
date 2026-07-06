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

    // 拷贝构造函数：用 other 创建一个新对象
    IntBuffer(const IntBuffer& other)
        : size_(other.size_), data_(new int[other.size_]) {
        std::cout << "copy construct, from "
                  << static_cast<void*>(other.data_)
                  << " to "
                  << static_cast<void*>(data_) << std::endl;

        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
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
    a.set(1, 200);

    IntBuffer b = a;  // 调用拷贝构造函数

    b.set(0, 999);

    std::cout << "a[0] = " << a.get(0) << std::endl;
    std::cout << "b[0] = " << b.get(0) << std::endl;

    return 0;
}