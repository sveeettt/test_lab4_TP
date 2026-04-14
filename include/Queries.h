#ifndef QUERIES_H
#define QUERIES_H

#include <sqlite3.h>
#include <string>

int callback(void* data, int argc, char** argv, char** azColName);

void showSoldAndRemaining(sqlite3* db);

void showSalesByDiscAndPeriod(sqlite3* db, int disk_id, const std::string& startDate, const std::string& endDate);

void showMostSoldDiscDetails(sqlite3* db);

void showTopPerformerSales(sqlite3* db);

void showAuthorStats(sqlite3* db);

#endif