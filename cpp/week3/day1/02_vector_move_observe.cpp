#include <iostream>
#include <utility>
#include <vector>

class Item {
public:
    explicit Item(int id) : id_(id) {
        std::cout << "construct id=" << id_ << '\n';
    }

    Item(const Item& other) : id_(other.id_) {
        std::cout << "copy construct id=" << id_ << '\n';
    }

    Item(Item&& other) noexcept : id_(other.id_) {
        std::cout << "move construct id=" << id_ << '\n';
        other.id_ = -1;
    }

    ~Item() {
        std::cout << "destruct id=" << id_ << '\n';
    }

private:
    int id_;
};

int main() {
    std::vector<Item> items;

    for (int id = 1; id <= 5; ++id) {
        std::cout << "\n--- before emplace id=" << id << " ---\n";
        std::cout
            << "size=" << items.size()
            << " capacity=" << items.capacity()
            << '\n';

        items.emplace_back(id);

        std::cout
            << "after: size=" << items.size()
            << " capacity=" << items.capacity()
            << '\n';
    }

    return 0;
}