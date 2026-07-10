#include <iostream>
#include <memory>
#include <utility>

int main() {
    auto p1 = std::make_unique<int>(42);
    auto p2 = std::make_unique<int>(42);
    p2=std::move(p2);
    if (p1 == nullptr) {
        std::cout << "p1 is null\n";
    }

    if (p2 != nullptr) {
        std::cout << "p2 value=" << *p2 << '\n';
    }

    return 0;
}