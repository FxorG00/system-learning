#include "buffer.hpp"
Buffer::Buffer(std::size_t initial_capacity) {
    data_.reserve(initial_capacity);
}

std::size_t Buffer::readable_bytes() const noexcept{
    return data_.size()-offset;
}
bool Buffer::empty() const noexcept{
    return readable_bytes()==0;
}

void Buffer::reset() {
    data_.clear();
    offset=0;
}

const char* Buffer::peek() const noexcept{
    if(readable_bytes()>0) {
        return data_.data()+offset;
    } else {
        return nullptr;
    }
}

std::size_t Buffer::tail_space() {
    return data_.capacity()-data_.size();
}

std::size_t Buffer::used() {
    return readable_bytes();
}

void Buffer::compact() {
    // 移动元素并让 offset=0
    // 注意移动后要进行 resize()，因为原先的元素还会占着位置
    // 这样才能保证我们的 readable_bytes 是正确的
    // 然后之后 push_back 也是正确去增长 size
    if(offset==0) {
        // 意味着此时无须任何移动，并且可以处理边界
        return ;
    }
    std::size_t used_=used();
    std::memmove(data_.data(),data_.data()+offset,used_);
    offset=0;
    data_.resize(used_);
}

void Buffer::append(const char* data, std::size_t length){
    if(tail_space()>=length) {
        for(std::size_t i=0;i<length;i++) {
            data_.push_back(data[i]);
        }
    } else {
        // 先压缩，再 push_back
        compact();
        for(std::size_t i=0;i<length;i++) {
            data_.push_back(data[i]);
        }
    }
}
void Buffer::retrieve(std::size_t length){
    if(length>readable_bytes()) {
        throw std::out_of_range("retrieve length exceeds readable bytes");
    }
    offset+=length;
    if(offset==data_.size()) {
        reset();
    }
}
std::string Buffer::retrieve_as_string(std::size_t length){
    if(length>readable_bytes()) {
        throw std::out_of_range("retrieve length exceeds readable bytes");
    }
    std::string result="";
    for(std::size_t i=offset;i<offset+length;i++) {
        result+=data_[i];
    }
    retrieve(length);
    return result;
}
std::string Buffer::retrieve_all_as_string(){
    return retrieve_as_string(readable_bytes());
}