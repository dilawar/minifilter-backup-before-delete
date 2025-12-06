cmake_minimum_required(VERSION 3.1)
project(BackupOnDeleteCTL LANGUAGES NONE)

find_program(NIM_EXEC NAMES nim nim.exe HINTS /mingw64/bin)
message(STATUS "NIM_EXEC = ${NIM_EXEC}")


## execute_process(
##     COMMAND ${NIM_EXEC} cpp --noLinking --nimcache:nim_source_dir "subconsciousctl.nim"
##     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
##     )
##
## file(GLOB nim_sources nim_source_dir/*.cpp)
##
## add_executable(subconscious-ctl)
## target_sources(subconscious-ctl PRIVATE ${nim_sources})
