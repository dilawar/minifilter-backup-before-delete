/**
 * Shield client.
 *
 */

#include "ShieldClient.h"

#include <deque>
#include <iomanip>
#include <chrono>
#include <iostream>
#include <thread>
#include <filesystem>

#include "../utils/utility.h"

#include "plog/Log.h"

using namespace std;
using namespace std::chrono_literals;

namespace fs = std::filesystem;

// Prefix is D then the file must be deleted.
constexpr wchar_t DELETE_CODE = L'D';

//
// Global deque to hold backedup files.
//
std::deque<pair<fs::path, fs::path>> files_to_backup_;

ShieldClient::ShieldClient(const ::wstring& portname,
                           const fs::path& configfile)
  : portname_(portname)
  , configfile_(configfile)
  , port_(INVALID_HANDLE_VALUE)
  , connected_(false)
  , stop_(false)
  , db_(make_unique<SqliteDb>(fs::temp_directory_path() / "shield.db"))
  , backblaze_(make_unique<BackBlazeClient>("0028ded07a0a3ad0000000001",
                                            "K002IAnOuukf1zBTdw/alTHqVnDO5C4"))
{}

ShieldClient::~ShieldClient()
{
    if (INVALID_HANDLE_VALUE != port_) {
        PLOGI << "Closing client port." << endl;
        CloseHandle(port_);
    }
}

void
ShieldClient::load_config()
{
    static auto last_config_load_time = time_now();

    if (!fs::exists(configfile_)) {
        PLOGW << "Configuration file " << configfile_.string()
              << " does not exists. Not loading." << endl;
        return;
    }

    // Load config file if it has changed recently.
    auto mtime = get_file_timestamp(configfile_);

    if (mtime > last_config_load_time) {
        // load now
        try {
            string data;
            read_file(configfile_, data);
            config_ = json::parse(data);
            last_config_load_time = time_now();
        } catch (std::exception& e) {
            LOGW << "Failed to parse config file " << configfile_.string()
                 << ": Error " << e.what() << endl;
            return;
        }
    }
}

void
ShieldClient::get_message()
{
    HRESULT hr;
    PSHIELD_MESSAGE message = NULL;
    LPOVERLAPPED pOvlp = NULL;
    DWORD outSize;
    ULONG_PTR key;
    BOOL success = FALSE;

    wchar_t msg_code = L'0';

    while (!stop_) {

        PSHIELD_MESSAGE pMsg = reinterpret_cast<PSHIELD_MESSAGE>(
          HeapAlloc(GetProcessHeap(), 0, sizeof(SHIELD_MESSAGE)));

        if (NULL == pMsg) {
            PLOGE << "Couldn't allocate memory for SHIELD_MESSAGE" << endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        FillMemory(&pMsg->Ovlp, sizeof(OVERLAPPED), 0);

        hr = FilterGetMessage(port_,
                              &pMsg->header,
                              FIELD_OFFSET(SHIELD_MESSAGE, Ovlp),
                              &pMsg->Ovlp);

        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
            hr = S_OK;

        //
        // Read message from the shield.
        //
        if (hr != S_OK) {
            PLOGW << "Failed to get message from shield. code: " << std::hex
                  << hr;

            HeapFree(GetProcessHeap(), 0, pMsg);

            //
            // E_HANDLE means that Handle is not valid (most likely,
            // disconnected). We retry connecting.
            //
            if (hr == E_HANDLE) {
                connected_ = false;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            continue;
        }

        message = NULL;

        //
        // Get overlapped structure asynchronously which was previously pumped
        // by FilterGetMessage
        //
        auto success = GetQueuedCompletionStatus(
          completion_port_, &outSize, &key, &pOvlp, 10 /* timeout in ms */);

        if (!success) {

            // hr = HRESULT_FROM_WIN32(GetLastError());

            // if (hr == E_HANDLE) {
            //     hr = S_OK;
            // }

            // else if (hr == HRESULT_FROM_WIN32(ERROR_ABANDONED_WAIT_0)) {
            //     hr = S_OK;
            // }

            continue;
        }

        message = CONTAINING_RECORD(pOvlp, SHIELD_MESSAGE, Ovlp);

        //
        // Free the memory that was initially allocated for message.
        //
        if (message) {

            msg_code = message->buffer[0];

            if (msg_code == L'0') {
                PLOGI << "Its a heartbeat.";
                continue;
            }

            if (msg_code == L'B') {

                //
                // Successfully got a filename to backup. When
                // the backup is complete, notify the minifilter as well.
                //
                std::wstring _from, _to;
                auto validpath =
                  buffer_to_paths(message->buffer + 1, _from, _to);

                if (!validpath) {
                    PLOGW << "Not a valid path " << _from;
                    continue;
                }

                //
                // Check of oldpath exists. It must.
                //
                fs::path existing = _from;
                if (!fs::exists(existing)) {
                    PLOGW << "Does not exists " << existing.string();
                    continue;
                }

                PLOGI << "Adding to backup list: " << existing.wstring();
                files_to_backup_.push_back({ existing, fs::path(_to) });
            } else {
                PLOGW << " Unsupported message code " << msg_code;
            }

            HeapFree(GetProcessHeap(), 0, message);
        }
    }
}

void
ShieldClient::connect()
{
    port_ = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;

#ifdef COMMUNICATION_IN_SYNC_MODE
    //
    // port in sync mode. We don't have to use Overlapped structure here.
    // TODO: Not sure about the performance.
    //
    PLOGI << "Connecting to " << portname_ << " in SYNC mode.";
    hr = FilterConnectCommunicationPort(
      portname_.c_str(), FLT_PORT_FLAG_SYNC_HANDLE, NULL, 0, NULL, &port_);
#else
    // port in async mode. This is the preferred way. Use with completion port.
    PLOGI << "Connecting to " << portname_ << " in ASYNC mode.";
    hr = FilterConnectCommunicationPort(
      portname_.c_str(), 0, NULL, 0, NULL, &port_);
#endif

    if (FAILED(hr)) {
        PLOGW << "Failed to connect to Shield: Error: 0x" << std::hex << hr;
        connected_ = false;
        goto MAIN_EXIT;
    }

    completion_port_ =
      CreateIoCompletionPort(port_, NULL, 0, 2 /* 2 user threads */);

    if (NULL == completion_port_) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        PLOGW << "Error in creating completion port. Code: 0x" << std::hex
              << hr;
        goto MAIN_EXIT;
    }

    PLOGI << " ... Successfully connected.";
    connected_ = true;

MAIN_EXIT:
    if (!connected_ && INVALID_HANDLE_VALUE != port_) {
        CloseHandle(port_);
    }
}

bool
ShieldClient::is_connected() const
{
    return connected_;
}

bool
ShieldClient::buffer_to_paths(const wchar_t* buf,
                              std::wstring& from,
                              std::wstring& to)
{
    // Filename starts with .^^^. and ends with .^^^
    size_t _pre_index = 0, _post_index = 0;

    bool _start = false;
    bool _valid = false;

    for (size_t i = 4; i < MESSAGE_LENGTH; ++i) {

        if (_start) {

            //
            // Searching for last .^^^ at the end.
            //
            if ((buf[i] == L'^') && (buf[i - 1] == L'^') &&
                (buf[i - 2] == L'^') && (buf[i - 3] == L'.')) {
                _valid = true;
                _start = false;
                _post_index = i;
                break;
            }
        } else {

            //
            // Starting at the loop: Searching for .^^^.
            //
            if ((buf[i] == L'.') && (buf[i - 1] == L'^') &&
                (buf[i - 2] == L'^') && (buf[i - 3] == L'^') &&
                (buf[i - 4] == L'.')) {
                _start = true;
                _pre_index = i;
            }
        }
    }

    if (_valid) {

        // Put the path information into strings. Second argument is count.
        from.append(buf, 0, _post_index + 1);

        to.append(buf, 0, _pre_index - 4);
        to.append(buf, _pre_index + 1, _post_index - _pre_index - 4);
    }

    return _valid;
}

bool
ShieldClient::notify_minifilter(const wstring& msg) const
{
    DWORD lpBytesReturned;

    PLOGI << "Sending to minifilter '" << msg << "'";

    auto hres = FilterSendMessage(port_,
                                  (void*)msg.data(),
                                  sizeof(wchar_t) * msg.size(),
                                  NULL,
                                  0,
                                  &lpBytesReturned);

    if (hres != S_OK) {
        PLOGW << "Failed to notify minifilter: Error code " << std::hex << hres;
        return false;
    }

    PLOGI << "Sent to minifilter: " << msg
          << ". size=" << sizeof(wchar_t) * msg.size()
          << " (returned size: " << lpBytesReturned << " bytes.)" << endl;
    return true;
}

bool
ShieldClient::backup(const fs::path& existing, const fs::path& orignal)
{
    return backblaze_->upload(existing, orignal);
}

/**
 * @brief Run the client.
 */
void
ShieldClient::run()
{
    HRESULT hr;

    if (!connected_)
        connect();

    //
    // Read messages from the minifilter in a separate thread. This push new
    // filepaths to backup to a deque.
    //
    // NOTE: Don't use [&] in capture.  See
    // https://community.osr.com/discussion/comment/303009
    //
    auto t = std::thread([=] { return this->get_message(); });
    t.detach();
    PLOGI << "Launched the consumer thread to read message from minifilter.";

    //
    // Loop over
    //
    while (!stop_) {

        if (files_to_backup_.empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }

        auto f = files_to_backup_.front();

        //
        // Backup only if file exists on the disk.
        //
        if (!fs::exists(f.first)) {
            PLOGW << f.first.string() << " does not exists.";
            files_to_backup_.pop_front();
            continue;
        }

        PLOGI << "Trying backup " << f.first.string();

#ifdef ENABLE_DATABASE
        // Add entry to database.
        int rowid = -1;
        try {
            rowid = db_->AddFilePendingBackup(f.first);
        } catch (exception& e) {
            PLOGW << "Failed to add to database: " << e.what();
        }
#endif

        auto ret = backup(f.first, f.second);

        if (ret) {

            //
            // If backup is successful, notify minifilter. If minifilter
            // deletes the file, remove it from the list.
            //
            ret = notify_minifilter(DELETE_CODE + f.first.wstring());
            if (ret)
                files_to_backup_.pop_front();

#ifdef ENABLE_DATABASE
            //
            // Also mark it successfully uploaded in the database.
            //
            db_->UpdateUploadStatusOfRowID(rowid);
#endif
        } else
            PLOGW << "Failed to backup " << f.first.wstring();
    }
}
