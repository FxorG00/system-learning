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
    items.reserve(3);  // 避免三次插入之间发生扩容，隔离新元素构造行为

    Item first(1);

    std::cout << "\n--- push_back lvalue ---\n";
    items.push_back(first);

    std::cout << "\n--- push_back rvalue ---\n";
    items.push_back(Item(2));

    std::cout << "\n--- emplace_back ---\n";
    items.emplace_back(3);

    std::cout << "\n--- erase even numbers ---\n";
    std::vector<int> numbers{1, 2, 3, 4, 5, 6};

    for (auto it = numbers.begin(); it != numbers.end();) {
        if (*it % 2 == 0) {
            it = numbers.erase(it);
        } else {
            ++it;
        }
    }

    for (int value : numbers) {
        std::cout << value << ' ';
    }
    std::cout << '\n';

    return 0;
}