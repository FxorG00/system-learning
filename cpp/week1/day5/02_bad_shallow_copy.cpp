#include <iostream>
#include <cstddef>

class IntBuffer {
public:
    explicit IntBuffer(std::size_t size)
        : size_(size), data_(new int[size]) {
        std::cout << "construct, data = "
                  << static_cast<void*>(data_) << std::endl;
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

    IntBuffer b = a;  // 默认拷贝：浅拷贝，危险

    std::cout << "a[0] = " << a.get(0) << std::endl;
    std::cout << "b[0] = " << b.get(0) << std::endl;

    return 0;
}