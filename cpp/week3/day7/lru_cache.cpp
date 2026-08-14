#include <iostream>
#include <list>
#include <unordered_map>
struct node {
    int key,value;
};
using li=std::list<node>::iterator;

class LRUCache {
private:
    std::size_t capacity_,size_;
    std::list<node>data_;
    std::unordered_map<int,li>umap_; 
    bool full() const {
        return capacity_==size_;
    }
    void pop_LRU() {
        auto it=prev(data_.end());
        int key=it->key;
        data_.pop_back();
        umap_.erase(umap_.find(key));
        --size_;
    }
public:
    explicit LRUCache(std::size_t capacity):
    capacity_(capacity),size_(0),data_(),umap_() {
    }

    bool get(int key, int& value) {
        auto umap_it=umap_.find(key);
        if(umap_it==umap_.end()) return 0;
        auto it=umap_it->second;
        // 现在 it 是 list 的迭代器，对应元素就是想要的
        value=it->value;
        // 把当前元素变成 MRU，也就是 front
        auto nw=*it;
        data_.erase(it);
        data_.push_front(nw);
        // 记得更改 mapping，因为原有迭代器失效了。
        (*umap_it).second=data_.begin();
        return 1;
    }
    void put(int key, int value) {
        if(capacity_==0) return ;
        auto umap_it=umap_.find(key);
        if(umap_it==umap_.end()) {
            if(full()) {
                // 已经满的话，需要清 LRU
                pop_LRU();
            }
            node nw={key,value};
            data_.push_front(nw);
            umap_[key]=data_.begin();
            ++size_;
        } else {
            auto it=umap_[key];
            data_.erase(it);
            data_.push_front(node{key,value});
            umap_[key]=data_.begin();
        }
    }
    std::size_t size() const {
        return size_;
    }
};

void test1() {
    LRUCache my_lru(2);
    my_lru.put(1,10);
    my_lru.put(2,20);
    int value;
    my_lru.get(1,value);
    std::cout<<value<<'\n';
    my_lru.put(3,30);
    std::cout<<"size: "<<my_lru.size()<<'\n';
    if(my_lru.get(2,value)) {
        std::cout<<"get(2) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(2) failed\n";
    }
    if(my_lru.get(1,value)) {
        std::cout<<"get(1) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(1) failed\n";
    }
    if(my_lru.get(3,value)) {
        std::cout<<"get(3) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(3) failed\n";
    }
}

void test2() {
    LRUCache my_lru(2);
    my_lru.put(1,10);
    my_lru.put(2,20);
    my_lru.put(1,15);
    my_lru.put(3,30);
    int value;
    if(my_lru.get(1,value)) {
        std::cout<<"get(1) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(1) failed\n";
    }
    if(my_lru.get(2,value)) {
        std::cout<<"get(2) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(2) failed\n";
    }
    if(my_lru.get(3,value)) {
        std::cout<<"get(3) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(3) failed\n";
    }
    std::cout<<my_lru.size()<<'\n';
}

void test3() {
    LRUCache my_lru(2);
    my_lru.put(1,10);
    my_lru.put(2,20);
    int value=0;
    my_lru.get(999,value);
    my_lru.put(3,30);
    if(my_lru.get(1,value)) {
        std::cout<<"get(1) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(1) failed\n";
    }
    if(my_lru.get(2,value)) {
        std::cout<<"get(2) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(2) failed\n";
    }
    if(my_lru.get(3,value)) {
        std::cout<<"get(3) successfully "<<value<<'\n';
    } else {
        std::cout<<"get(3) failed\n";
    }
}

int main() {
    test1();
    test2();
    test3();
    return 0;
}