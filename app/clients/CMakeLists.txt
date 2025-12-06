# CPR for sending requests. Its a wrapper around curl.
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git 
    GIT_TAG 1.6.2)

set(CPR_BUILD_TESTS OFF)
FetchContent_MakeAvailable(cpr)

add_library(clients STATIC BackBlazeClient.cpp)
target_link_libraries(clients PUBLIC cpr::cpr)
