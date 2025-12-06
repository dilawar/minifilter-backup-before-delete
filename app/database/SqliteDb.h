//
//    Description: SqliteDb of files and config.
//
//         Author:  Dilawar Singh (), dilawar.s.rajput@gmail.com
//   Organization:  Dilawar Singh
//

#ifndef DATABASE_H_KF3GIKPR
#define DATABASE_H_KF3GIKPR

#include <filesystem>
#include <memory>
#include <tuple>
#include <vector>

#include "SQLiteCpp/SQLiteCpp.h"

using namespace std;

namespace fs = std::filesystem;

class SqliteDb
{
  public:
    SqliteDb();
    SqliteDb(const fs::path& dbpath);

    ~SqliteDb();

    void Initialize();

    void Connect();
    void Disconnect();

    size_t AddFilePendingBackup(fs::path& filepath);

    int ChangeUpdateStatusOfRowId(const int rowid, const int status);
    bool UpdateUploadStatusOfRowID(const unsigned long rowid);

    void GetListOfFilesByUploadStatus(const int code,
                                      vector<tuple<int, string>>& files,
                                      const unsigned int limit);

    int DeleteRowByRowid(int rowid);

    std::string dbpath(bool absolute=true) const
    {
        if(absolute)
            return fs::absolute(dbpath_).string();
        return dbpath_.string();
    }

  private:
    /* data */
    fs::path dbpath_;

    std::unique_ptr<SQLite::Database> db_;

    bool connected_;
    bool initialized_;
};

#endif /* end of include guard: DATABASE_H_KF3GIKPR */
