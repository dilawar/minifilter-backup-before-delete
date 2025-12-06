cmake_minimum_required(VERSION 3.15)

project(APP LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

#
# Options
#
option(ENABLE_TESTS "Enable tests" ON)

#
# Global
#
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)

#
# TODO: Add keyid/secret here and create a config file that can be added to the
# build system.
#
# Client specific values.
#
set(BACKBLAZE_KEY_ID "")
set(BACKBLAZE_SECRET "")

#
# Find required dependencies
#

# install using chocolaty.
find_package(WindowsSDK COMPONENTS tools REQUIRED)
find_package(WDK REQUIRED)

# install boost using msys2 (too complicated for FetchContent).
find_package(Boost REQUIRED)
message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

include(FetchContent)

# plog and fmt for formatting and logging. 
FetchContent_Declare(plog GIT_REPOSITORY https://github.com/SergiusTheBest/plog
    GIT_TAG 1.1.5)
FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt
    GIT_TAG 8.0.1)
FetchContent_MakeAvailable(fmt plog)

#
# Global include directories.
#
include_directories(${plog_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(ENABLE_TESTS)
    # doctest for testing. 
    FetchContent_Declare(doctest GIT_REPOSITORY https://github.com/onqtam/doctest
        GIT_TAG 2.4.6)
    FetchContent_MakeAvailable(doctest)
    include_directories(${doctest_SOURCE_DIR})
endif()

#
# Compile the usermode app.
#
add_subdirectory(thirdparty)

#
# Add static libraries
#
add_subdirectory(clients)
add_subdirectory(database)
add_subdirectory(utils)

add_executable(BackupOnDeleteBackup src/main.cpp src/ShieldClient.cpp)
target_link_libraries(BackupOnDeleteBackup PUBLIC clients database utils)
target_link_libraries(BackupOnDeleteBackup PRIVATE ctrlc FltLib)

#
# Set the properties on target.
# Using generator expression is really weired here but that how it seems to
# work in 2021(https: // stackoverflow.com/a/45871818/1805129)
set_target_properties(BackupOnDeleteBackup
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<0:>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<0:>")

install(TARGETS BackupOnDeleteBackup DESTINATION Shield)

#
# Tests
#
enable_testing()
add_subdirectory(tests)
