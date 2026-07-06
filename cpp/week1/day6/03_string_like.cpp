/*
需求：
    从 const char* 构造
    析构释放内存
    拷贝构造深拷贝
    拷贝赋值深拷贝
    c_str() 返回内部字符串
    size() 返回长度
使用：
    std::strlen(s)
    std::memcpy(dst, src, n)
    strlen：计算 C 字符串长度，不包含结尾 '\0'
    memcpy：按字节复制内存
Author: 
    FxorG
*/
#include <iostream>
#include <cstddef>
#include <cstring>

class StringLike {
public:
    StringLike(const char* data): len_(strlen(data)+1),data_(new char[len_]) {
        memcpy(data_,data,len_);
    }
    StringLike(const StringLike& other) {
        char* new_data=new char[other.len_];
        data_=new_data;
        len_=other.len_;
        memcpy(data_,other.data_,other.len_);
    }
    StringLike& operator =(const StringLike& other) {
        if(this==&other) {
            return *this;
        }
        char* new_data=new char[other.len_];
        memcpy(new_data,other.data_,other.len_);
        delete[] data_;
        data_=new_data;
        len_=other.len_;
        return *this;
    }
    ~StringLike() {
        delete[] data_;
    }
    std::size_t size() const {
        return len_-1;
    }
    const char* c_str() const {
        return data_;
    }
private:
    std::size_t len_;
    char* data_;
};

int main() {
    StringLike a("abc");
    StringLike b("hello world");
    a = b;
    std::cout << a.c_str() << " " << a.size() << std::endl;
    return 0;
}