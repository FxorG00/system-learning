# CMake generated Testfile for 
# Source directory: /home/xgf/code/system-learning/cpp/week8
# Build directory: /home/xgf/code/system-learning/cpp/week8/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/xgf/code/system-learning/cpp/week8/build/thread_pool_test[1]_include.cmake")
include("/home/xgf/code/system-learning/cpp/week8/build/async_logger_test[1]_include.cmake")
add_test(component_demo_smoke "/home/xgf/code/system-learning/cpp/week8/build/component_demo")
set_tests_properties(component_demo_smoke PROPERTIES  TIMEOUT "20" _BACKTRACE_TRIPLES "/home/xgf/code/system-learning/cpp/week8/CMakeLists.txt;94;add_test;/home/xgf/code/system-learning/cpp/week8/CMakeLists.txt;0;")
