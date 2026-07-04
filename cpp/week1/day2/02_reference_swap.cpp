#include <iostream>

void swap_by_pointer(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void swap_by_reference(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}

int main() {
    int x = 10;
    int y = 20;

    std::cout << "before swap: x = " << x << ", y = " << y << std::endl;

    swap_by_pointer(&x, &y);
    std::cout << "after pointer swap: x = " << x << ", y = " << y << std::endl;

    swap_by_reference(x, y);
    std::cout << "after reference swap: x = " << x << ", y = " << y << std::endl;

    return 0;
}