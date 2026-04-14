#include "Auth.h"
#include <iostream>

bool authenticate(sqlite3* db, const std::string& username, const std::string& password, User& user) {
    std::string sql = "SELECT id, username, password, role FROM users WHERE username = '" + username + "' AND password = '" + password + "';";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Query preparation error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool authorize(const User& user, const std::string& required_role) {
    if (required_role == "admin") {
        return user.role == "admin";
    }
    if (required_role == "user") {
        return (user.role == "admin" || user.role == "user");
    }
    return false;
}