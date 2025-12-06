/**
 * Backblaze client.
 */

#include "cpr/cpr.h"
#include "BackBlazeClient.h"
#include "../utils/utility.h"

#define MAX_TRIALS 5

//
// PLOG header
//
#include "plog/Log.h"

template<typename T = string>
T
__get_json(const json& json, const string& key)
{
    try {
        return T(json.at(key));
    } catch (exception& e) {
        LOGW << "Error: " << key << " not found." << endl;
    }

    return T();
}

BackBlazeClient::BackBlazeClient(const string& keyid, const string& secret)
  : keyid_(keyid)
  , secret_(secret)
  , authenticated_(false)
{
    init();
}

BackBlazeClient::~BackBlazeClient()
{}

void
BackBlazeClient::init()
{
    if (!authenticated_)
        authenticated_ = authenticate();
}

json
BackBlazeClient::get_upload_url(const json& auth_headers)
{
    auto endpoint =
      __get_json(auth_headers, "apiUrl") + "/b2api/v2/b2_get_upload_url";

    auto token = __get_json(auth_headers, "authorizationToken");
    auto bucket = auth_headers.at("allowed").at("bucketId");
    auto params = cpr::Parameters{ { "bucketId", bucket } };

    for (size_t i = 1; i <= MAX_TRIALS; ++i) {

        auto r = cpr::Get(cpr::Url{ endpoint },
                          cpr::Header{ { "Authorization", token } },
                          params);

        if (r.status_code == 200) // Success.
            return json::parse(r.text);

        PLOGW << "Failed to get unique url to upload. I will try "
              << MAX_TRIALS - i << " more times." << endl;
    }

    return json();
}

json
BackBlazeClient::upload_file(const json& rjson,
                             const fs::path& existing,
                             const fs::path& original)
{
    auto bucket = rjson.at("bucketId");
    auto endpoint = rjson.at("uploadUrl");
    auto token = rjson.at("authorizationToken");

    //
    // Read the file and compute sha1.
    //
    std::string data;
    const size_t filesize = read_file(existing, data);
    const string sha1 = get_sha1(data);

    //
    // Upload now.
    //
    const string filename = url_encode(original.string());

    PLOGD << "Uploading " << existing.string() << " to " << original.string()
          << endl
          << " endpoint: " << endpoint << endl;

    for (size_t i = 1; i <= MAX_TRIALS; ++i) {

        auto r =
          cpr::Post(cpr::Url{ endpoint },
                    cpr::Body{ data },
                    cpr::Header{ { "Authorization", token },
                                 { "X-Bz-File-Name", filename },
                                 { "X-Bz-Content-Sha1", sha1 },
                                 { "X-Bz-Info-Author", "unknown" },
                                 { "X-Bz-Server-Size-Encryption", "AES256" } },
                    cpr::Parameters{ { "bucketId", bucket } });

        if (r.status_code == 200) // Success uploading.
            return json::parse(r.text);

        PLOGW << "Could not upload. I will try " << MAX_TRIALS - i
              << " times more before giving up." << endl;
    }
    return json();
}

bool
BackBlazeClient::upload(const fs::path& existing, const fs::path& original)
{

    //
    // If not authenticated, authenticate. This is usually called once.
    //
    if (!authenticated_)
        authenticate();

    if (!fs::exists(existing)) {
        PLOGW << "File '" << existing.wstring()
              << "' does not exist. Can't create backup." << endl;
        return false;
    }

    PLOGI << " Trying uploading to BackBlaze client. "<< existing.string();

    //
    // Find the upload URL.
    //
    auto rjson = get_upload_url(auth_headers_);
    if (rjson.empty()) {
        PLOGW << "Could not fetch upload URL after trying " << MAX_TRIALS
              << " times. Giving up." << endl;
    }

    //
    // We got the unique upload URL from b2 API. Continue with upload now...
    //
    rjson = upload_file(rjson, existing, original);
    if (rjson.empty()) {
        PLOGW << "Could not upload file. Tried " << MAX_TRIALS
              << " times. Giving up." << endl;
        return false;
    }

    PLOGI << "     successfully uploaded.";
    return true;
}

bool
BackBlazeClient::authenticate()
{
    if (!(keyid_.size() > 0 || secret_.size() > 0)) {
        PLOGW << "Authentication information is not set. " << endl;
        return false;
    }

    //
    // Now authneticate and get the API tokens.
    //
    const auto r = cpr::Get(cpr::Url{ endpoint_ + "b2_authorize_account" },
                            cpr::Authentication{ keyid_, secret_ },
                            cpr::Header{ { "accept", "application/json" } });

    //
    // On success, parse the returned headers. They are used when uploading
    // the file.
    //
    if (r.status_code == 200)
        auth_headers_ = json::parse(r.text);

    authenticated_ = r.status_code == 200;
    return authenticated_;
}
