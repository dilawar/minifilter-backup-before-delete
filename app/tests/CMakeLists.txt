set(TEST_FILES test_sha1.cpp)

foreach(_test_file ${TEST_FILES})
    message(STATUS "[TEST] adding ${_test_file}")
    get_filename_component(_test_name ${_test_file} NAME_WE)
    add_executable(${_test_name} ${_test_file})
    target_link_libraries(${_test_name} PRIVATE utils Boost::headers)
    target_include_directories(${_test_name} PRIVATE ${doctest_SOURCE_DIR})
    add_test(NAME test_${_test_name}
        COMMAND $<TARGET_FILE:${_test_name}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

#
# Nim tests
#
find_program(NIM_EXECUTABLE NAMES nim
    PATHS /mingw64 /clang64
    PATH_SUFFIXES bin
    REQUIRED)

message(STATUS "NIM_EXECUTABLE = ${NIM_EXECUTABLE}")

set(TEST_LIVE_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/_test_live.exe)

add_custom_target(_test_live ALL DEPENDS ${TEST_LIVE_EXECUTABLE})
add_custom_command(OUTPUT ${TEST_LIVE_EXECUTABLE}
    DEPENDS test_live.nim
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/build_nim.sh test_live.nim
    COMMENT "Compiling test_live.nim"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME test_live COMMAND ${TEST_LIVE_EXECUTABLE})
