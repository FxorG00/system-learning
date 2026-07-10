#include <iostream>
#include <memory>

class Resource {
public:
    explicit Resource(int id) : id_(id) {
        std::cout << "Resource construct id=" << id_ << '\n';
    }

    ~Resource() {
        std::cout << "Resource destruct id=" << id_ << '\n';
    }

    int id() const {
        return id_;
    }

private:
    int id_;
};

int main() {
    auto p1 = std::make_shared<Resource>(1);
    std::cout << "count after p1=" << p1.use_count() << '\n';

    {
        auto p2 = p1;
        std::cout << "count after p2=" << p1.use_count() << '\n';
        std::cout << "p2 id=" << p2->id() << '\n';
    }

    std::cout << "count after p2 dead=" << p1.use_count() << '\n';
    return 0;
}