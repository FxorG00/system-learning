#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Task {
    int deadline;
    std::string name;
};

void print_map_range(
    const std::map<int, std::string>& tasks,
    int begin,
    int end) {
    const auto first = tasks.lower_bound(begin);
    const auto last = tasks.upper_bound(end);

    std::cout << "map range [" << begin << ", " << end << "]:";
    for (auto it = first; it != last; ++it) {
        std::cout << ' ' << it->first << ':' << it->second;
    }
    std::cout << '\n';
}

int main() {
    // 场景一：数据一次构建、之后主要查询，vector 很合适。
    std::vector<Task> batch{
        {30, "flush"},
        {10, "heartbeat"},
        {20, "timeout"}
    };

    std::sort(
        batch.begin(),
        batch.end(),
        [](const Task& lhs, const Task& rhs) {
            return lhs.deadline < rhs.deadline;
        });

    const auto batch_it = std::lower_bound(
        batch.begin(),
        batch.end(),
        18,
        [](const Task& task, int deadline) {
            return task.deadline < deadline;
        });

    if (batch_it != batch.end()) {
        std::cout
            << "vector first deadline >= 18: "
            << batch_it->deadline << ' ' << batch_it->name
            << '\n';
    }

    // 场景二：数据持续修改，并且始终需要有序和范围查询。
    std::map<int, std::string> dynamic_tasks{
        {30, "flush"},
        {10, "heartbeat"},
        {20, "timeout"}
    };

    auto saved = dynamic_tasks.find(20);

    dynamic_tasks.emplace(25, "retry");
    dynamic_tasks.erase(10);

    if (saved != dynamic_tasks.end()) {
        std::cout
            << "saved iterator after other insert/erase: "
            << saved->first << ' ' << saved->second
            << '\n';
    }

    const auto first_not_before = dynamic_tasks.lower_bound(24);
    if (first_not_before != dynamic_tasks.end()) {
        std::cout
            << "map first deadline >= 24: "
            << first_not_before->first << ' '
            << first_not_before->second
            << '\n';
    }

    print_map_range(dynamic_tasks, 20, 29);

    return 0;
}