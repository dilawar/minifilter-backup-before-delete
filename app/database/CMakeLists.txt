# Sqlite3 for filepath db and config management.
FetchContent_Declare(SQLite
    URL https://sqlite.org/2021/sqlite-amalgamation-3360000.zip
    URL_HASH SHA3_256=d25609210ec93b3c8c7da66a03cf82e2c9868cfbd2d7d866982861855e96f972)

FetchContent_Declare(SQLiteCPP
    GIT_REPOSITORY https://github.com/BackupOnDeleteCompute/SQLiteCpp
    GIT_TAG master)

set(SQLITECPP_RUN_CPPLINT OFF)
FetchContent_MakeAvailable(SQLite SQLiteCPP)

add_library(database STATIC SqliteDb.cpp ${sqlite_SOURCE_DIR}/sqlite3.c)
target_link_libraries(database PUBLIC fmt::fmt SQLiteCpp)
target_include_directories(database PUBLIC ${sqlitecpp_SOURCE_DIR}/include)

#
# Tests
#
if(ENABLE_TESTS)

    add_executable(_test_db test_db.cpp)
    target_link_libraries(_test_db PUBLIC database)

    enable_testing()
    add_test(NAME test_db COMMAND $<TARGET_NAME:_test_db>)

endif()
