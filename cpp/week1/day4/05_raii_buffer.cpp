#include <iostream>
#include <cstddef>

class IntBuffer {
public:
    // 禁止这个构造函数被用于隐式类型转换
    explicit IntBuffer(std::size_t size)
        : size_(size), data_(new int[size]) {
        std::cout << "IntBuffer construct, size = " << size_ << std::endl;
    }

    ~IntBuffer() {
        std::cout << "IntBuffer destruct, size = " << size_ << std::endl;
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

    std::size_t size() const {
        return size_;
    }

    // 今天先禁止拷贝，避免引出深拷贝 / 浅拷贝问题。
    // =delete 是明确禁止某个函数被使用
    // 禁止 IntBuffer b2=b1，利用 b1 来拷贝构造 b2
    IntBuffer(const IntBuffer&) = delete;
    // 禁止拷贝赋值，也就是 b2=b1 是被禁止的
    // 因为默认拷贝是浅拷贝，就是全都赋值成一样，这样会导致 double delete
    IntBuffer& operator=(const IntBuffer&) = delete;

private:
    std::size_t size_;
    int* data_;
};

void use_buffer() {
    IntBuffer buffer(5);

    buffer.set(0, 100);
    buffer.set(1, 200);

    std::cout << "buffer[0] = " << buffer.get(0) << std::endl;
    std::cout << "buffer[1] = " << buffer.get(1) << std::endl;

    // 不需要手动 delete[]。
    // 函数结束时 buffer 自动析构，析构函数里会 delete[] data_。
}

int main() {
    std::cout << "enter main" << std::endl;

    use_buffer();

    std::cout << "leave main" << std::endl;
    return 0;
}