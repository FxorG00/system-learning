#include <iostream>

class A {
public:
    int val;
    A(int id=0):val(id) {
        std::cout<<"create "<<val<<std::endl;
    }
    ~A() {
        std::cout<<"delete "<<val<<std::endl;
    }
};

int main() {
    A* p=new A[3];
    p[0].val=4;
    p[1].val=5;
    p[2].val=6;
    delete[] p;
    return 0;
}