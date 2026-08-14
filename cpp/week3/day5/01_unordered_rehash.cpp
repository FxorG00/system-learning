#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>

using Table = std::unordered_map<int, std::string>;

void print_state(const Table& table, const char* stage) {
    std::cout
        << stage
        << " size=" << table.size()
        << " bucket_count=" << table.bucket_count()
        << " load_factor=" << table.load_factor()
        << " max_load_factor=" << table.max_load_factor()
        << '\n';
}

int main() {
    Table connections;
    connections.max_load_factor(0.1F);
    connections.reserve(4);

    connections.emplace(1, "client-1");
    connections.emplace(2, "client-2");
    connections.emplace(3, "client-3");

    print_state(connections, "before rehash");

    auto saved = connections.find(1);
    const std::string* saved_value_address = &saved->second;
    const std::size_t old_bucket_count = connections.bucket_count();

    int next_fd = 100;
    while (connections.bucket_count() == old_bucket_count) {
        connections.emplace(next_fd, "extra-client");
        ++next_fd;
    }

    print_state(connections, "after rehash");

    // saved 已因 rehash 失效，不能再使用；重新 find 获取迭代器。
    const auto fresh = connections.find(1);
    std::cout
        << "value address unchanged=" << std::boolalpha
        << (&fresh->second == saved_value_address)
        << '\n';

    std::unordered_map<int, int> prepared;
    prepared.max_load_factor(0.7F);
    prepared.reserve(100);
    const std::size_t prepared_bucket_count = prepared.bucket_count();

    for (int key = 0; key < 100; ++key) {
        prepared.emplace(key, key * 10);
    }

    std::cout
        << "reserved table size=" << prepared.size()
        << " bucket_count unchanged="
        << (prepared.bucket_count() == prepared_bucket_count)
        << '\n';

    return 0;
}