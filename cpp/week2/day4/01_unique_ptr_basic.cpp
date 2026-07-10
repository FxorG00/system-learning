#include <iostream>
#include <memory>

class Resource {
public:
    explicit Resource(int value) : value_(value) {
        std::cout << "Resource construct " << value_ << '\n';
    }

    ~Resource() {
        std::cout << "Resource destruct " << value_ << '\n';
    }

    int value() const {
        return value_;
    }

private:
    int value_;
};

int main() {
    auto p = std::make_unique<Resource>(42);
    std::cout << p->value() << '\n';
    return 0;
}