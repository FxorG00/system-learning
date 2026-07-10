#include <iostream>
#include <vector>

class Item {
public:
    Item() {
        std::cout << "construct\n";
    }

    Item(const Item&) {
        std::cout << "copy construct\n";
    }

    Item(Item&&) {
        std::cout << "move construct\n";
    }
};

int main() {
    std::vector<Item> v;
    v.reserve(1);
    v.emplace_back();
    v.emplace_back();
    return 0;
}