// =====================================================================================
//
//       Filename:  utility.hpp
//
//    Description: Utility functions.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  Dilawar Singh
//
// =====================================================================================

#ifndef UTILITY_HPP_UKOYTY0M
#define UTILITY_HPP_UKOYTY0M

#include <string>
#include <ostream>
#include <iomanip>
#include <fstream>
#include <filesystem>

using namespace std;

namespace fs = std::filesystem;

string
url_encode(const string& value);

std::string
get_sha1(const std::string& p_arg);

size_t
read_file(const fs::path& filepath, string& data);

string
get_sha1_file(const fs::path& filepath);

std::error_code
delete_file(const fs::path& filepath);

std::time_t
time_now();

std::time_t
get_file_timestamp(const fs::path& path);

#endif /* end of include guard: UTILITY_HPP_UKOYTY0M */
