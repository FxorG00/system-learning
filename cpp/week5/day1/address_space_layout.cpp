/*
目标：
    观察当前进程中几类对象的虚拟地址，并与 /proc/<pid>/maps 对照。

验证方法：
    1. 编译并运行程序。
    2. 保持程序暂停，在另一个终端执行：
       cat /proc/<程序打印出的 pid>/maps
    3. 检查各地址落入哪一个 virtual address range。

注意：
    程序打印出的都是当前进程的 virtual address。
    /proc/<pid>/maps 也展示 virtual mappings，不展示真实 physical address。
*/

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>

// 已初始化的全局对象通常位于程序的可写数据 mapping 中。
int initialized_global = 10;

// 零初始化全局对象通常位于 BSS 对应的可写区域中。
// BSS 是 Block Started by Symbol 的历史名称。
int zero_initialized_global;

// 统一打印“对象名称 + 地址”，避免 main 中重复输出代码。
void print_address(const char* name, const void* address) {
    std::cout << name << " = " << address << '\n';
}

int main() {
    static int static_local = 20;
    int stack_local = 30;
    auto heap_value = std::make_unique<int>(40);

    // sysconf：system configuration，在运行时查询系统配置值。
    // _SC_PAGESIZE：查询当前系统的 page size，单位是 byte。
    const long page_size_value = ::sysconf(_SC_PAGESIZE);
    if (page_size_value <= 0) {
        std::fprintf(
            stderr,
            "sysconf(_SC_PAGESIZE) returned an invalid value\n"
        );
        return 1;
    }

    const std::size_t page_size =
        static_cast<std::size_t>(page_size_value);

    // 创建一页 private anonymous mapping：
    // nullptr：让 kernel 选择 virtual address；
    // PROT_READ | PROT_WRITE：允许读写；
    // MAP_PRIVATE | MAP_ANONYMOUS：私有匿名映射；
    // -1：匿名映射不使用普通文件 fd；
    // 0：file offset；匿名映射下保持为 0。
    void* const mapping = ::mmap(
        nullptr,
        page_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (mapping == MAP_FAILED) {
        ::perror("mmap");
        return 1;
    }

    // 访问映射中的第一个 byte，证明该 virtual address 可以正常读写。
    // “第一次访问一页时还可能发生什么”留到 Day4 的 page fault 再解释。
    auto* const mapped_bytes = static_cast<char*>(mapping);
    mapped_bytes[0] = 'M';

    std::cout
        << "pid = " << ::getpid()
        << ", page size = " << page_size
        << '\n';

    print_address("initialized global", &initialized_global);
    print_address("zero initialized global", &zero_initialized_global);
    print_address("static local", &static_local);
    print_address("heap object", heap_value.get());
    print_address("stack local", &stack_local);
    print_address("anonymous mmap", mapping);

    // 保持进程存活，另一个终端才能读取这个 pid 对应的 maps。
    std::cout
        << "Open another terminal and run:\n"
        << "  cat /proc/" << ::getpid() << "/maps\n"
        << "Press Enter here to unmap and exit.\n"
        << std::flush;

    std::cin.get();

    // 释放 mmap 建立的 mapping。unique_ptr 管理的 heap object 会自动释放。
    if (::munmap(mapping, page_size) == -1) {
        ::perror("munmap");
        return 1;
    }

    return 0;
}