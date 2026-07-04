#include <iostream>
#include <string>

void print_by_value(std::string s) {
    std::cout << "by value: " << s << std::endl;
}

void print_by_const_ref(const std::string& s) {
    std::cout << "by const ref: " << s << std::endl;
    // s = "changed"; // 取消注释会编译错误
}

void change_by_ref(std::string& s) {
    s = "changed";
}

int main() {
    std::string name = "fxorg";

    print_by_value(name);
    std::cout << "after print_by_value: " << name << std::endl;

    print_by_const_ref(name);
    std::cout << "after print_by_const_ref: " << name << std::endl;

    change_by_ref(name);
    std::cout << "after change_by_ref: " << name << std::endl;

    return 0;
}