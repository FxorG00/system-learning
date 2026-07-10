#include <iostream>
#include <memory>
#include <string>
#include <utility>

class Person {
public:
    explicit Person(std::string name) : name_(std::move(name)) {
        std::cout << "Person construct " << name_ << '\n';
    }

    ~Person() {
        std::cout << "Person destruct " << name_ << '\n';
    }

    void set_friend(const std::shared_ptr<Person>& other) {
        friend_ = other;
    }

    void print_friend() const {
        auto p = friend_.lock();
        if (p != nullptr) {
            std::cout << name_ << " friend is " << p->name_ << '\n';
        } else {
            std::cout << name_ << " friend is gone\n";
        }
    }

private:
    std::string name_;
    std::weak_ptr<Person> friend_;
};

int main() {
    auto a = std::make_shared<Person>("A");
    auto b = std::make_shared<Person>("B");

    a->set_friend(b);
    b->set_friend(a);

    a->print_friend();
    b->print_friend();

    return 0;
}