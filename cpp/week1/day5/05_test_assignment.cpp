#include <iostream>
#include <cstddef>

int main() {
    int a=5,b=10,c=20;
    (a=b)=c;
    std::cout<<a<<" "<<b<<" "<<c<<std::endl;
    return 0;
}