/**
 * Client for BackupOnDeleteShield
 */

#ifndef SHIELDCLIENT_H_IXPX4WKN
#define SHIELDCLIENT_H_IXPX4WKN

#include <Windows.h>
#include <fltuser.h>

#include <string>
#include <memory>

#include "../clients/BackBlazeClient.h"
#include "../database/SqliteDb.h"

//
// JSON support
//
#include "../thirdparty/nlohmann/json.hpp"

using nlohmann::json;

//
// Message recieved from Shild. It contains a header and path of renamed file.
//

constexpr size_t MESSAGE_LENGTH = 260;

typedef struct _SHIELD_MESSAGE
{
    FILTER_MESSAGE_HEADER header;
    wchar_t buffer[MESSAGE_LENGTH];
    OVERLAPPED Ovlp;
} SHIELD_MESSAGE, *PSHIELD_MESSAGE;

//
// This class is responsible for creating backup.
//

class ShieldClient
{
  public:
    ShieldClient(const std::wstring& portname, const fs::path& configpath);

    ~ShieldClient();

    bool buffer_to_paths(const wchar_t* buf,
                         std::wstring& existing,
                         std::wstring& original);

    void load_config();

    bool notify_minifilter(const wstring& msg) const;

    bool backup(const fs::path& existing, const fs::path& original);

    void get_message();

    void connect();

    void run();

    bool is_connected() const;

  private:
    /* data */
    std::wstring portname_;

    fs::path configfile_;
    json config_;

    HANDLE port_;
    HANDLE completion_port_;

    bool connected_;
    bool stop_;

    std::unique_ptr<SqliteDb> db_;
    std::unique_ptr<BackBlazeClient> backblaze_;
};

#endif /* end of include guard: SHIELDCLIENT_H_IXPX4WKN */
