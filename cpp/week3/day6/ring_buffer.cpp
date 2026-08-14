#include <iostream>
#include <vector>

class RingBuffer {
private:
    std::size_t capacity_,size_;
    std::vector<int>values_;
    std::size_t head_,tail_;
    // next: 去找到一个位置在 ring buffer 中的下一个位置
    std::size_t next(std::size_t pos) const {
        if(pos+1<capacity_) return pos+1;
        return 0;
    }
public:
    explicit RingBuffer(std::size_t capacity):
    capacity_(capacity),size_(0),values_(capacity),head_(0),tail_(0) {
    }

    // 0: push 失败
    bool push(int value) {
        if(full()) return 0;
        values_[tail_]=value;
        tail_=next(tail_);
        ++size_;
        return 1;
    }
    bool pop(int& value) {
        if(empty()) return 0;
        value=values_[head_];
        head_=next(head_);
        --size_;
        return 1;
    }

    bool empty() const {
        if(capacity_==0) return 1;
        return size_==0;
    }
    bool full() const {
        if(capacity_==0) return 1;
        return size_==capacity_;
    }
    std::size_t size() const {
        return size_;
    }
    std::size_t capacity() const {
        return capacity_;
    }
};

void test1() {
    RingBuffer my_buffer(3);
    if(my_buffer.push(10)) {
        std::cout<<"push 10 successfully\n";
    } else {
        std::cout<<"push 10 failed\n";
    }
    if(my_buffer.push(20)) {
        std::cout<<"push 20 successfully\n";
    } else {
        std::cout<<"push 20 failed\n";
    }
    if(my_buffer.push(30)) {
        std::cout<<"push 30 successfully\n";
    } else {
        std::cout<<"push 30 failed\n";
    }
    if(my_buffer.push(40)) {
        std::cout<<"push 40 successfully\n";
    } else {
        std::cout<<"push 40 failed\n";
    }
    std::cout<<"push over --\n";
    
    int value=0;
    if(my_buffer.pop(value)) {
        std::cout<<"pop succesfully: "<<value<<'\n';
    } else { 
        std::cout<<"pop failed\n";
    }
    if(my_buffer.push(40)) {
        std::cout<<"push 40 successfully\n";
    }
    if(my_buffer.pop(value)) {
        std::cout<<"pop succesfully: "<<value<<'\n';
    } else { 
        std::cout<<"pop failed\n";
    }
    if(my_buffer.pop(value)) {
        std::cout<<"pop succesfully: "<<value<<'\n';
    } else { 
        std::cout<<"pop failed\n";
    }
    if(my_buffer.pop(value)) {
        std::cout<<"pop succesfully: "<<value<<'\n';
    } else { 
        std::cout<<"pop failed\n";
    }
}

void test2() {
    RingBuffer my_buffer(1);
    std::cout<<"before push "<<my_buffer.empty()<<' '<<my_buffer.full()<<'\n';
    std::cout<<my_buffer.push(7)<<'\n';
    std::cout<<"after push "<<my_buffer.empty()<<' '<<my_buffer.full()<<'\n';
    std::cout<<my_buffer.push(8)<<'\n';
    int value;
    my_buffer.pop(value);
    std::cout<<value<<'\n';
    std::cout<<"after pop "<<my_buffer.empty()<<' '<<my_buffer.full()<<'\n';
}

void test3() {
    RingBuffer my_buffer(0);
    std::cout<<my_buffer.empty()<<' '<<my_buffer.full()<<'\n';
    std::cout<<my_buffer.push(10)<<'\n';
    int value=0;
    std::cout<<my_buffer.pop(value)<<'\n';
}

int main() {
    test1();
    test2();
    test3();
    return 0;
}