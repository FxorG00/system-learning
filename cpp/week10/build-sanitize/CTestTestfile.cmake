# CMake generated Testfile for 
# Source directory: /home/xgf/code/system-learning/cpp/week10
# Build directory: /home/xgf/code/system-learning/cpp/week10/build-sanitize
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/xgf/code/system-learning/cpp/week10/build-sanitize/buffer_test[1]_include.cmake")
include("/home/xgf/code/system-learning/cpp/week10/build-sanitize/channel_test[1]_include.cmake")
add_test(event_loop_probe "/home/xgf/code/system-learning/cpp/week10/build-sanitize/event_loop_probe")
set_tests_properties(event_loop_probe PROPERTIES  _BACKTRACE_TRIPLES "/home/xgf/code/system-learning/cpp/week10/CMakeLists.txt;98;add_test;/home/xgf/code/system-learning/cpp/week10/CMakeLists.txt;0;")
add_test(acceptor_probe "/home/xgf/code/system-learning/cpp/week10/build-sanitize/acceptor_probe")
set_tests_properties(acceptor_probe PROPERTIES  _BACKTRACE_TRIPLES "/home/xgf/code/system-learning/cpp/week10/CMakeLists.txt;133;add_test;/home/xgf/code/system-learning/cpp/week10/CMakeLists.txt;0;")
add_test(connection_checker "/home/xgf/code/system-learning/cpp/week10/build-sanitize/connection_checker")
set_tests_properties(connection_checker PROPERTIES  TIMEOUT "15" _BACKTRACE_TRIPLES "/home/xgf/code/system-learning/cpp/week10/CMakeLists.txt;174;add_test;/home/xgf/code/system-learning/cpp/week10/CMakeLists.txt;0;")
