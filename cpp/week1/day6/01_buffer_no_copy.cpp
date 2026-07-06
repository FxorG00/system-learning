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
    Buffer(size_t len): data_(new char[len]),len_(len) {

    }
    Buffer(const Buffer& other) = delete;
    Buffer& operator =(const Buffer& other)=delete;
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
        if(index>=len_||index<0) {
            return ' ';
        }
        return data_[index];
    }
    size_t size() {
        return len_;
    }

private:
    char* data_;
    size_t len_;
};

int main() {   
    Buffer a(5);
    std::cout<<a.size()<<std::endl;
    return 0;
}