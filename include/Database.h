#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>

class Database {
private:
    sqlite3* db;
    std::string db_path;
public:
    Database(const std::string& filename);
    ~Database();
    bool connect();
    bool updateSTOCKFromTRANSACTIONs();  
    sqlite3* getDB() { return db; }
    bool isConnected() { return db != nullptr; }
};

#endif