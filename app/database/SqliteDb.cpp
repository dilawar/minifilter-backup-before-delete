//
//    Description: SqliteDb class.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  Dilawar Singh
//

#include "SqliteDb.h"
#include "fmt/core.h"
#include "plog/Log.h"

#include <memory>
#include <iostream>

#define RETURN_IF_NOT_CONNECTED0                                               \
    if (!connected_) {                                                         \
        PLOGW << "Database is not connected.";                                 \
        return;                                                                \
    }

#define RETURN_IF_NOT_CONNECTED(X)                                             \
    if (!connected_) {                                                         \
        PLOGW << "Database is not connected.";                                 \
        return X;                                                              \
    }

using namespace std;

SqliteDb::SqliteDb()
  : dbpath_(fs::path("shield.db"))
  , connected_(false)
{
    Connect();
}

SqliteDb::SqliteDb(const fs::path& dbpath)
  : dbpath_(dbpath)
  , connected_(false)
  , initialized_(false)
{
    Connect();
}

SqliteDb::~SqliteDb()
{
    Disconnect(); // its a noop.
}

void
SqliteDb::Initialize()
{
    RETURN_IF_NOT_CONNECTED0;

    try {

        db_->exec("CREATE TABLE IF NOT EXISTS files ("
                  "_id INTEGER PRIMARY KEY ASC, "
                  "path VARCHAR(512) NOT NULL, "
                  "upload_status TINYINT DEFAULT -1, "
                  "upload_num_tries INT DEFAULT 0, "
                  "last_tried_on TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                  ")");
        initialized_ = true;
        PLOGI << "Successfully initialized.";
    } catch (std::exception& e) {

        PLOGW << "Failed to initialize " << e.what();
        initialized_ = false;
    }
}

void
SqliteDb::Connect()
{
    PLOGI << "Opening database " << dbpath();
    try {

        db_ = make_unique<SQLite::Database>(dbpath(),
                                            SQLite::OPEN_READWRITE |
                                              SQLite::OPEN_CREATE |
                                              SQLite::OPEN_FULLMUTEX);

        db_->setBusyTimeout(10); // 100ms timeout.
        connected_ = true;

        Initialize();

    } catch (std::exception& e) {
        connected_ = false;
        PLOGW << "Failed to open database " << dbpath()
              << ". Error: " << e.what() << endl;
        return;
    }
}

void
SqliteDb::Disconnect()
{
    //
    // Nothing to do here. SQLite::SqliteDb takes care of closing the
    // connection in RAII fashion.
    //
}

size_t
SqliteDb::AddFilePendingBackup(fs::path& filepath)
{
    RETURN_IF_NOT_CONNECTED(0);

    bool success = false;
    unsigned long rowid = 0;
    
    PLOGI << "Adding to pending list " << filepath.string();

    //
    // insert a filepath that is to be backedup.
    //
    try {
        db_->exec(fmt::format("INSERT INTO files (path) VALUES (\"{}\");",
                              filepath.string()));
        rowid = db_->getLastInsertRowid();
    } catch (std::exception& e) {
        PLOGW << "Failed to insert a file that is to be uploaded. Error: "
              << e.what() << "." << endl;
        rowid = 0;
    }
    PLOGI << " rowid = " << rowid;
    return rowid;
}

int
SqliteDb::ChangeUpdateStatusOfRowId(const int rowid, const int status)
{
    RETURN_IF_NOT_CONNECTED(-1);

    auto n = db_->exec(fmt::format(
      "UPDATE files SET upload_status='{}' WHERE _id='{}';", status, rowid));
    return n;
}

bool
SqliteDb::UpdateUploadStatusOfRowID(const unsigned long rowid)
{
    RETURN_IF_NOT_CONNECTED(false);

    if (rowid < 1) {
        PLOGW << "Invalid row id " << rowid;
        return false;
    }

    bool success = false;

    try {
        ChangeUpdateStatusOfRowId(rowid, 0);
        success = true;
        PLOGI << "Row id " << rowid << " is successfully marked 'UPLOADED'";
    } catch (std::exception& e) {
        PLOGW << "Failed to mark file as 'UPLOADED'." << e.what() << ".";
    }
    return success;
}

void
SqliteDb::GetListOfFilesByUploadStatus(const int code,
                                       vector<tuple<int, string>>& files,
                                       const unsigned int limit = 100)
{
    RETURN_IF_NOT_CONNECTED0;

    SQLite::Statement query(
      *db_, "SELECT * FROM files WHERE upload_status=? LIMIT ?");

    query.bind(1, code);
    query.bind(2, limit);

    while (query.executeStep())
        files.push_back({ query.getColumn(0), query.getColumn(1) });
}

int
SqliteDb::DeleteRowByRowid(int rowid)
{
    RETURN_IF_NOT_CONNECTED(-1);

    SQLite::Statement query(*db_, "DELETE FROM files WHERE _id=?");
    query.bind(1, rowid);

    try {
        return query.exec();
    } catch (std::exception& e) {
        PLOGW << "Failed to delete row with id " << rowid << ". Error was "
              << e.what();
        return -1;
    }
}
