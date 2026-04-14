#include "Database.h"
#include <iostream>

Database::Database(const std::string& filename) : db_path(filename), db(nullptr) {
}

Database::~Database() {
    if (db) {
        sqlite3_close(db);
        std::cout << "Connection closed" << std::endl;
    }
}

bool Database::connect() {
    if (sqlite3_open(db_path.c_str(), &db)) {
        std::cerr << "Error: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
        return false;
    }
    std::cout << "Connected to: " << db_path << std::endl;
    return true;
}

bool Database::updateSTOCKFromTRANSACTIONs() {
    const char* sqlDelete = "DELETE FROM STOCK;";
    char* errMsg = nullptr;

    if (sqlite3_exec(db, sqlDelete, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    const char* sqlInsert = R"(
        INSERT INTO STOCK (disc_id, total_income, total_sold, remaining)
        SELECT 
            disc_id,
            SUM(CASE WHEN operation_type = 'income' THEN quantity ELSE 0 END),
            SUM(CASE WHEN operation_type = 'sale' THEN quantity ELSE 0 END),
            SUM(CASE WHEN operation_type = 'income' THEN quantity ELSE 0 END) -
            SUM(CASE WHEN operation_type = 'sale' THEN quantity ELSE 0 END)
        FROM TRANSACTION
        GROUP BY disc_id;
    )";

    if (sqlite3_exec(db, sqlInsert, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    std::cout << "Stock updated" << std::endl;
    return true;
}