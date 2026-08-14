#include <cstddef>
#include <iostream>
#include <string>
#include <unordered_map>

struct ConstantHash {
    std::size_t operator()(int) const noexcept {
        return 0;
    }
};

struct CountingEqual {
    inline static std::size_t comparisons = 0;

    bool operator()(int lhs, int rhs) const noexcept {
        ++comparisons;
        return lhs == rhs;
    }
};

using BadTable = std::unordered_map<
    int,
    std::string,
    ConstantHash,
    CountingEqual>;

int main() {
    BadTable table;
    table.max_load_factor(100.0F);
    table.rehash(1);

    for (int key : {10, 20, 30, 40, 50}) {
        table.emplace(key, "value");
    }

    const std::size_t bucket_index = table.bucket(10);
    std::cout
        << "size=" << table.size()
        << " bucket_count=" << table.bucket_count()
        << " shared_bucket=" << bucket_index
        << " bucket_size=" << table.bucket_size(bucket_index)
        << '\n';

    std::cout << "keys in shared bucket:";
    for (auto it = table.begin(bucket_index);
         it != table.end(bucket_index);
         ++it) {
        std::cout << ' ' << it->first;
    }
    std::cout << '\n';

    CountingEqual::comparisons = 0;
    const auto found = table.find(30);
    std::cout
        << "find 30=" << std::boolalpha
        << (found != table.end())
        << " equality comparisons=" << CountingEqual::comparisons
        << '\n';

    CountingEqual::comparisons = 0;
    const auto missing = table.find(999);
    std::cout
        << "find 999=" << (missing != table.end())
        << " equality comparisons=" << CountingEqual::comparisons
        << '\n';

    return 0;
}