// =====================================================================================
//
//       Filename:  test_sha1.cpp
//
//    Description: Testing Sha1.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  Dilawar Singh
//
// =====================================================================================

#include "plog/Log.h"

#include <filesystem>
#include <iostream>

#include "../utils/utility.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

namespace fs = std::filesystem;

using namespace std;

TEST_CASE("test sha1")
{
    string b = "abcd1234";
    string h = get_sha1(b);
    REQUIRE(h == "7ce0359f12857f2a90c7de465f40a95f01cb5da9");

    b = "1234567890^&*(";
    h = get_sha1(b);
    REQUIRE(h == "ae673165e403ee1439694a3d726e9fea7cd4a1e8");
}

TEST_CASE("test sha1 of file")
{
    string data;
    read_file(fs::path("../tests/c-hotkeys.pdf"), data);
    PLOGI << " Total bytes read : " << data.size() << endl;
    string h = get_sha1(data);
    REQUIRE(h == "c527dfaa80ce2f8d6ca8e90f22dfea6ae3b2a0d1");
}

