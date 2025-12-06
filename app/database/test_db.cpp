// =====================================================================================
//
//       Filename:  test_db.cpp
//
//    Description:  Testing database connection.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  Dilawar Singh
//
// =====================================================================================
//

#include "SqliteDb.h"
#include "fmt/ranges.h"

#include <filesystem>
namespace fs = std::filesystem;

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

TEST_CASE("test db")
{
    fs::path dbpath = "test.db";
    if (fs::exists(dbpath))
        fs::remove(dbpath);

    SqliteDb db(dbpath);

    constexpr size_t LIMIT = 100;

    //
    // Check the number of files to be backed up.
    //
    vector<tuple<int, string>> entries;
    db.GetListOfFilesByUploadStatus(-1, entries, LIMIT);
    size_t size1 = entries.size();
    REQUIRE(size1 == 0);

    //
    // Add two files and access them.
    //
    entries.clear();
    fs::path filetoadd = fs::temp_directory_path() / "test1.txt";
    auto rowid = db.AddFilePendingBackup(filetoadd);

    filetoadd = fs::temp_directory_path() / "test2.txt";
    rowid = db.AddFilePendingBackup(filetoadd);
    db.GetListOfFilesByUploadStatus(-1, entries, LIMIT);

    fmt::print("\n==={}\n", entries);


    size_t size2 = entries.size();

    REQUIRE(min(size2, LIMIT) == min(size1 + 2, LIMIT));

    //
    // delete the added file.
    //
    std::cout << "\n=== Deleting row by id " << rowid << std::endl;
    auto n = db.DeleteRowByRowid(rowid);
    REQUIRE(n == 1); // 1 row is modified.

    entries.clear();
    db.GetListOfFilesByUploadStatus(-1, entries, 100);
    size_t size3 = entries.size();
    fmt::print("{}\n", entries);

    REQUIRE(min(size3, LIMIT) == min(size2 - n, LIMIT));
}

