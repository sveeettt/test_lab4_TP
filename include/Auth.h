#ifndef AUTH_H
#define AUTH_H

#include <sqlite3.h>
#include <string>

struct User {
    int id;
    std::string username;
    std::string password;
    std::string role;
};

bool authenticate(sqlite3* db, const std::string& username, const std::string& password, User& user);
bool authorize(const User& user, const std::string& required_role);

#endif