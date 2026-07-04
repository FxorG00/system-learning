#include <iostream>
#include <string>

class Tracer {
public:
    Tracer(const std::string& name)
        : name_(name) {
        std::cout << "construct: " << name_ << std::endl;
    }

    ~Tracer() {
        std::cout << "destruct: " << name_ << std::endl;
    }

    void hello() const {
        std::cout << "hello from " << name_ << std::endl;
    }

private:
    std::string name_;
};

int main() {
    std::cout << "enter main" << std::endl;

    Tracer a("a");
    a.hello();

    {
        std::cout << "enter inner scope" << std::endl;
        Tracer b("b");
        b.hello();
        std::cout << "leave inner scope" << std::endl;
    }

    std::cout << "leave main" << std::endl;
    return 0;
}