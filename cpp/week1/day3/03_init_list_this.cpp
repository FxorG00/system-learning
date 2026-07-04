#include <iostream>
#include <string>
using namespace std;

class User {
public:
    User(const string& name): name_(name) {
        
    }
    void print() {
        cout<<"in print() "<<this->name_<<'\n';
    }
    void print_const() const {
        cout<<"in print_const() "<<this->name_<<'\n';
    }
private:
    string name_;
};

int main() {
    User u1("FxorG");
    u1.print();
    return 0;
}