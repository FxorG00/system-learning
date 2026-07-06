/*
需求：
    Buffer 管理一块 char 数组
    构造函数申请内存
    析构函数释放内存
    提供 set / get / size
    禁止拷贝构造
    禁止拷贝赋值
Author: FxorG
*/
#include <iostream>
#include <cstddef>

class Buffer {
public:
    explicit Buffer(size_t len): data_(new char[len]),len_(len) {

    }
    Buffer(const Buffer& other): data_(new char[other.len_]),len_(other.len_) {
        for(size_t i=0;i<len_;i++) data_[i]=other.data_[i];
    }
    Buffer& operator =(const Buffer& other) {
        if(this==&other) {
            return *this;
        }
        char* new_data=new char[other.len_];
        for(size_t i=0;i<other.len_;i++) new_data[i]=other.data_[i];
        delete[] data_;
        len_=other.len_;
        data_=new_data;
        return *this;
    }
    ~Buffer() {
        delete[] data_;
    }
    void set(size_t index,char value) {
        if(index>=len_) {
            return ;
        }
        data_[index]=value;
    }
    char get(size_t index) {
        if(index>=len_) {
            return '\0';
        }
        return data_[index];
    }
    size_t size() const {
        // 加 const 意味着不会修改 *this 这个对象
        return len_;
    }

private:
    char* data_;
    size_t len_;
};

int main() {   
    Buffer a(5);
    a.set(2, 'p');

    Buffer b = a;   // 测拷贝构造
    b.set(2, 'x');
    std::cout<<a.get(2)<<" "<<b.get(2)<<std::endl;
    Buffer c(3);
    c = a;          // 测拷贝赋值
    std::cout<<c.get(2)<<std::endl;
    a = a;          // 测自赋值

    return 0;
}