find_package(Boost REQUIRED)

add_library(utils STATIC utility.cpp)
target_link_libraries(utils PUBLIC Boost::headers)
