cmake_minimum_required(VERSION 3.16)

project(Week10 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# buffer
add_library(buffer src/buffer.cpp)
add_executable(buffer_test tests/buffer_test.cpp)
target_compile_options(buffer PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(buffer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
target_link_libraries(buffer PUBLIC
    Threads::Threads
)
target_compile_options(buffer_test PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(buffer_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
include(GoogleTest)
gtest_discover_tests(buffer_test)
target_link_libraries(buffer_test PUBLIC
    Threads::Threads
    buffer
    GTest::gtest_main
    GTest::gtest
)

# channel
add_library(channel src/channel.cpp)
add_executable(channel_test tests/channel_test.cpp)
target_compile_options(channel PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(channel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
target_link_libraries(channel PUBLIC
    Threads::Threads
)
target_compile_options(channel_test PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(channel_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
include(GoogleTest)
gtest_discover_tests(channel_test)
target_link_libraries(channel_test PUBLIC
    Threads::Threads
    channel
    GTest::gtest_main
    GTest::gtest
)

# event_loop
add_library(event_loop src/event_loop.cpp)
add_executable(event_loop_probe tests/event_loop_probe.cpp)
target_compile_options(event_loop PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(event_loop PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
target_link_libraries(event_loop PUBLIC
    channel
    Threads::Threads
)
target_compile_options(event_loop_probe PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(event_loop_probe PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
add_test(
    NAME event_loop_probe
    COMMAND event_loop_probe
)
target_link_libraries(event_loop_probe PUBLIC
    Threads::Threads
    event_loop
    GTest::gtest_main
    GTest::gtest
)

# acceptor
add_library(acceptor src/acceptor.cpp)
target_compile_options(acceptor PRIVATE
    -Wall
    -Wextra
    -g
)
target_include_directories(acceptor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
target_link_libraries(acceptor PUBLIC
    event_loop
)

add_executable(acceptor_probe tests/acceptor_probe.cpp)
target_compile_options(acceptor_probe PRIVATE
    -Wall
    -Wextra
    -g
)
target_link_libraries(acceptor_probe PRIVATE
    acceptor
)

add_test(
    NAME acceptor_probe
    COMMAND acceptor_probe
)

# connection
add_library(connection src/connection.cpp)
target_include_directories(connection PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/reactor
)
target_compile_options(connection PRIVATE
    -Wall
    -Wextra
    -g
)
target_link_libraries(connection PUBLIC
    event_loop
    buffer
)

add_executable(reactor_echo_server apps/reactor_echo_server.cpp)
target_compile_options(reactor_echo_server PRIVATE
    -Wall
    -Wextra
    -g
)
target_link_libraries(reactor_echo_server PRIVATE
    acceptor
    connection
)

add_executable(connection_checker tests/connection_checker.cpp)
target_compile_options(connection_checker PRIVATE
    -Wall
    -Wextra
    -g
)
target_link_libraries(connection_checker PRIVATE
    connection
)

add_test(NAME connection_checker COMMAND connection_checker)
set_tests_properties(connection_checker PROPERTIES TIMEOUT 15)