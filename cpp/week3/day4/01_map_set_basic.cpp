#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <string>

int main() {
    std::map<std::string, int> word_count;

    ++word_count["redis"];
    ++word_count["reactor"];
    ++word_count["redis"];

    std::cout << "word_count in key order:\n";
    for (const auto& [word, count] : word_count) {
        std::cout << word << ' ' << count << '\n';
    }

    const std::size_t size_before = word_count.size();
    const auto missing = word_count.find("nginx");
    std::cout
        << "find nginx=" << std::boolalpha
        << (missing != word_count.end())
        << " size_changed="
        << (word_count.size() != size_before)
        << '\n';

    const int nginx_count = word_count["nginx"];
    std::cout
        << "operator[] value=" << nginx_count
        << " size=" << word_count.size()
        << '\n';

    std::set<int> ports{443, 22, 80, 80};
    const auto [it, inserted] = ports.insert(8080);
    const auto [same_it, inserted_again] = ports.insert(8080);

    std::cout
        << "inserted " << *it << '=' << inserted
        << " inserted again " << *same_it << '=' << inserted_again
        << '\n';

    std::cout << "ports in key order:";
    for (int port : ports) {
        std::cout << ' ' << port;
    }
    std::cout << '\n';

    return 0;
}