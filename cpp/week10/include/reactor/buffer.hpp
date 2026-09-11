#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring>
class Buffer {
public:
    explicit Buffer(std::size_t initial_capacity = 8);

    std::size_t readable_bytes() const noexcept;
    bool empty() const noexcept;
    const char* peek() const noexcept;

    void append(const char* data, std::size_t length);
    void retrieve(std::size_t length);
    std::string retrieve_as_string(std::size_t length);
    std::string retrieve_all_as_string();

private:
    // Round1 由你决定 representation，不在这里预先给成员。
    std::vector<char>data_;
    std::size_t offset=0;
    void reset();
    std::size_t tail_space();
    std::size_t used();
    void compact();
};