#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

int main() {
    std::string text = "GET /";

    std::cout
        << "text=" << text
        << " size=" << text.size()
        << " capacity=" << text.capacity()
        << " strlen(c_str)=" << std::strlen(text.c_str())
        << '\n';

    const std::size_t old_capacity = text.capacity();
    const std::uintptr_t old_address =
        reinterpret_cast<std::uintptr_t>(text.c_str());

    while (text.capacity() == old_capacity) {
        text.push_back('x');
    }

    const std::uintptr_t new_address =
        reinterpret_cast<std::uintptr_t>(text.c_str());

    std::cout
        << "after growth: size=" << text.size()
        << " capacity=" << text.capacity()
        << " old_address=" << old_address
        << " new_address=" << new_address
        << " address_changed=" << std::boolalpha
        << (old_address != new_address)
        << '\n';

    // 修改后重新获取 c_str()，不再使用旧指针。
    const char* fresh = text.c_str();
    std::cout << "fresh c_str=" << fresh << '\n';

    const std::size_t slash_pos = text.find('/');
    if (slash_pos != std::string::npos) {
        std::cout << "slash position=" << slash_pos << '\n';
    }

    const std::size_t missing_pos = text.find("POST");
    if (missing_pos == std::string::npos) {
        std::cout << "POST not found\n";
    }

    return 0;
}