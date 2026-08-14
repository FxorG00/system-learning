#include <cstddef>
#include <iostream>
#include <vector>

int main() {
    std::vector<int> values;

    const int* previous_data = values.data();

    for (int value = 0; value < 20; ++value) {
        const std::size_t old_capacity = values.capacity();

        values.push_back(value);

        const bool address_changed = values.data() != previous_data;
        // data() 就是 vector 的首地址
        const bool capacity_changed = values.capacity() != old_capacity;
        
        std::cout
            << "push=" << value
            << " size=" << values.size()
            << " capacity=" << values.capacity()
            << " data=" << static_cast<const void*>(values.data())
            << " address_changed=" << std::boolalpha << address_changed
            << " capacity_changed=" << capacity_changed
            << '\n';

        previous_data = values.data();
    }

    std::vector<int> numbers;

    numbers.reserve(8);
    std::cout
        << "after reserve: size=" << numbers.size()
        << " capacity=" << numbers.capacity()
        << '\n';

    numbers.resize(8);
    std::cout
        << "after resize: size=" << numbers.size()
        << " capacity=" << numbers.capacity()
        << '\n';
    return 0;
}