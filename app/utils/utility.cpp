// =====================================================================================
//
//       Filename:  utility.cpp
//
//    Description: Utilities.
//
//        Version:  1.0
//        Created:  10/13/21 13:24:42
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  Dilawar Singh
//
// =====================================================================================

#include "utility.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/detail/sha1.hpp>

using namespace std;

namespace fs = std::filesystem;

string
url_encode(const string& value)
{
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n;
         ++i) {
        string::value_type c = (*i);

        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
        escaped << nouppercase;
    }

    return escaped.str();
}

std::string
get_sha1(const std::string& p_arg)
{
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(p_arg.data(), p_arg.size());
    unsigned hash[5] = { 0 };
    sha1.get_digest(hash);

    // Back to string
    char buf[41] = { 0 };

    for (int i = 0; i < 5; i++) {
        std::sprintf(buf + (i << 3), "%08x", hash[i]);
    }

    return std::string(buf);
}

size_t
read_file(const fs::path& filepath, string& data)
{
    const size_t filesize = fs::file_size(filepath);
    std::ifstream inf(filepath.c_str(), std::ios::in | std::ios::binary);
    data.resize(filesize);
    inf.read(data.data(), filesize);
    return filesize;
}

string
get_sha1_file(const fs::path& filepath)
{
    string data;
    read_file(filepath, data);
    return get_sha1(data);
}

std::error_code
delete_file(const fs::path& filepath)
{
    std::error_code ec;
    fs::remove(filepath, ec);
    return ec;
}

std::time_t
time_now()
{
    return std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
}

std::time_t
get_file_timestamp(const fs::path& path)
{
#ifdef _WIN32
    struct _stat64 fileInfo;
    if (_wstati64(path.wstring().c_str(), &fileInfo) != 0) {
        throw std::runtime_error("Failed to get last write time.");
    }
    return fileInfo.st_mtime;

#else
    auto ftime = fs::last_write_time(path);
    return decltype(ftime)::clock::to_time_t(ftime);
#endif
}

