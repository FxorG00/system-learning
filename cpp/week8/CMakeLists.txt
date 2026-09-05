cmake_minimum_required(VERSION 3.16)

project(Week8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
add_library(async_logger src/async_logger.cpp)
add_executable(thread_pool_test tests/thread_pool_test.cpp)
add_executable(async_logger_test tests/async_logger_test.cpp)
add_executable(async_logger_bench benchmark/async_logger_bench.cpp)
target_compile_options(thread_pool_test PRIVATE
    -Wall
    -Wextra
    -g
)
target_compile_options(async_logger PRIVATE
    -Wall
    -Wextra
    -g
)
target_compile_options(async_logger_bench PRIVATE
    -Wall
    -Wextra
    -g
)
target_compile_options(async_logger_test PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(thread_pool_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(async_logger PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(async_logger_bench PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(async_logger PUBLIC
    Threads::Threads
)
target_link_libraries(async_logger_bench PRIVATE
    Threads::Threads
)
# CMake 3.16 的 FindGTest imported target names。
target_link_libraries(thread_pool_test PRIVATE
    GTest::GTest
    GTest::Main
    Threads::Threads
)
target_link_libraries(async_logger_test PRIVATE
    GTest::GTest
    GTest::Main
    Threads::Threads
    async_logger
)
target_link_libraries(async_logger_bench PRIVATE
    async_logger
)
include(GoogleTest)
gtest_discover_tests(thread_pool_test
    PROPERTIES TIMEOUT 10
)
gtest_discover_tests(async_logger_test
    PROPERTIES TIMEOUT 10
)

option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)

add_executable(component_demo
    demos/component_demo.cpp
)

target_link_libraries(component_demo
    PRIVATE
        async_logger
        Threads::Threads
)
target_include_directories(component_demo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_options(component_demo
    PRIVATE
        -Wall
        -Wextra
)
add_test(NAME component_demo_smoke COMMAND component_demo)
set_tests_properties(component_demo_smoke PROPERTIES TIMEOUT 20)

if(ENABLE_TSAN)
    target_compile_options(thread_pool_test PRIVATE
        -O1
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    target_link_options(thread_pool_test PRIVATE
        -fsanitize=thread
    )
    target_compile_options(async_logger_test PRIVATE
        -O1
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    target_link_options(async_logger_test PRIVATE
        -fsanitize=thread
    )
    target_compile_options(async_logger PRIVATE
        -O1
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    target_link_options(async_logger PRIVATE
        -fsanitize=thread
    )
    target_compile_options(async_logger_bench PRIVATE
        -O1
        -fsanitize=thread
        -fno-omit-frame-pointer
    )
    target_link_options(async_logger_bench PRIVATE
        -fsanitize=thread
    )
    target_compile_options(component_demo PRIVATE
        -O1 -fsanitize=thread -fno-omit-frame-pointer
    )
    target_link_options(component_demo PRIVATE
        -fsanitize=thread
    )
endif()