cmake_minimum_required(VERSION 3.12)

add_executable(test_rename ${CMAKE_CURRENT_SOURCE_DIR}/test_rename.cpp)
target_link_libraries(test_rename WDK::FLTMGR)
