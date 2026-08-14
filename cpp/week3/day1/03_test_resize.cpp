#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
    std::vector<int>vec;
    vec.reserve(100);
    
    vec[0]=1;
    std::cout<<vec.size()<<" "<<vec.capacity()<<'\n';
    return 0;
}