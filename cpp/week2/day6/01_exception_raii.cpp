#include <iostream>
#include <stdexcept>

class Tracer {
public:
    explicit Tracer(const char* name) : name_(name) {
        std::cout << "construct " << name_ << '\n';
    }

    ~Tracer() {
        std::cout << "destruct " << name_ << '\n';
    }

private:
    const char* name_;
};

void work() {
    Tracer a("a");
    Tracer b("b");

    std::cout << "before throw\n";
    throw std::runtime_error("work failed");

    std::cout << "after throw\n";
}

int main() {
    try {
        work();
    } catch (const std::exception& e) {
        std::cout << "catch: " << e.what() << '\n';
    }

    return 0;
}