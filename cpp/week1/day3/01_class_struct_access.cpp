#include <iostream>
#include <string>

struct Point {
    int x;
    int y;
};

class User {
public:
    User(const std::string& name, int age)
        : name_(name), age_(age) {}

    void print() const {
        std::cout << "name = " << name_ << ", age = " << age_ << std::endl;
    }

private:
    std::string name_;
    int age_;
};

int main() {
    Point p{1, 2};
    std::cout << "Point: " << p.x << ", " << p.y << std::endl;

    User u("fxorg", 19);
    u.print();

    // u.name_ = "gpt"; // 取消注释会编译错误，因为 name_ 是 private

    return 0;
}