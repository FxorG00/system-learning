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

    Tracer stack_obj("stack");
    stack_obj.hello();

    Tracer* heap_obj = new Tracer("heap");
    heap_obj->hello();

    std::cout << "before delete" << std::endl;
    delete heap_obj;
    std::cout << "after delete" << std::endl;

    std::cout << "leave main" << std::endl;
    return 0;
}