#ifndef BACKBLAZECLIENT_H_W31RH4BZ
#define BACKBLAZECLIENT_H_W31RH4BZ

#include <string_view>
#include <map>
#include <filesystem>

//
// JSON support
//
#include "../thirdparty/nlohmann/json.hpp"

using nlohmann::json;

using namespace std;
namespace fs = std::filesystem;

class BackBlazeClient
{
  public:
    BackBlazeClient(const string& keyid, const string& secret);

    ~BackBlazeClient();

    // ====================  ACCESSORS =======================================
    bool is_conntected();

    // ====================  MUTATORS =======================================
    void init();

    json get_upload_url(const json& json);

    json upload_file(const json& rjson,
                     const fs::path& existing,
                     const fs::path& original);

    bool upload(const fs::path& existing, const fs::path& original);

    // ====================  OPERATORS =======================================
    bool authenticate();

    bool connect();

  private:
    std::string keyid_;
    std::string secret_;

    bool authenticated_;

    //
    // Authentication info returned by backblaze API upon authentication.
    //
    json auth_headers_;

    const string endpoint_ = "https://api.backblazeb2.com/b2api/v2/";

}; // -----  end of class BackBlazeClient  -----

#endif /* end of include guard: BACKBLAZECLIENT_H_W31RH4BZ */
