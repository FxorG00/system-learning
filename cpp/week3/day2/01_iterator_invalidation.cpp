#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    std::vector<int> values;
    values.reserve(4);

    int next_value = 10;
    while (values.size() < values.capacity()) {
        values.push_back(next_value);
        next_value += 10;
    }

    auto old_it = values.begin();
    const int old_value = *old_it;
    const std::uintptr_t old_address =
        reinterpret_cast<std::uintptr_t>(values.data());
    const std::size_t old_capacity = values.capacity();

    std::cout
        << "before reallocation: value=" << old_value
        << " size=" << values.size()
        << " capacity=" << old_capacity
        << " data=" << old_address
        << '\n';

    values.push_back(999);  // size == capacity，必然需要更多存储

    const std::uintptr_t new_address =
        reinterpret_cast<std::uintptr_t>(values.data());

    std::cout
        << "after reallocation: size=" << values.size()
        << " capacity=" << values.capacity()
        << " data=" << new_address
        << " address_changed=" << std::boolalpha
        << (old_address != new_address)
        << '\n';

    // old_it 已失效，不能再解引用。

    std::vector<int> numbers{10, 20, 30, 40, 50};
    auto erase_pos = numbers.begin() + 1;
    auto next_it = numbers.erase(erase_pos);

    std::cout << "erase returned value=" << *next_it << '\n';
    std::cout << "after erase:";
    for (int value : numbers) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    return 0;
}