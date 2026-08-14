#include <algorithm>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

std::optional<int> find_first_greater_than(
    const std::vector<int>& values,
    int limit) {
    auto it = std::find_if(
        values.begin(),
        values.end(),
        [limit](int value) {
            return value > limit;
        });

    if (it == values.end()) {
        return std::nullopt;
    }

    return *it;
}

int main() {
    std::vector<int> values{7, 2, 9, 2, 5, 6, 4};

    std::sort(values.begin(), values.end());

    std::cout << "sorted:";
    for (int value : values) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    auto found = std::find(values.begin(), values.end(), 5);
    if (found != values.end()) {
        std::cout << "find 5 at index="
                  << std::distance(values.begin(), found)
                  << '\n';
    }

    auto lower = std::lower_bound(values.begin(), values.end(), 6);
    if (lower != values.end()) {
        std::cout << "first value >= 6 is " << *lower << '\n';
    }

    values.erase(
        std::remove_if(
            values.begin(),
            values.end(),
            [](int value) {
                return value % 2 == 0;
            }),
        values.end());

    std::cout << "after removing even numbers:";
    for (int value : values) {
        std::cout << ' ' << value;
    }
    std::cout << '\n';

    const auto result = find_first_greater_than(values, 7);
    if (result.has_value()) {
        std::cout << "first value > 7 is " << *result << '\n';
    } else {
        std::cout << "no value > 7\n";
    }

    std::pair<int, std::string> response{200, "OK"};
    auto [status_code, message] = response;
    std::cout << "response=" << status_code << ' ' << message << '\n';

    std::tuple<int, std::string, bool> user{7, "FxorG", true};
    auto [id, name, active] = user;
    std::cout
        << "user=" << id << ' ' << name
        << " active=" << std::boolalpha << active
        << '\n';

    return 0;
}