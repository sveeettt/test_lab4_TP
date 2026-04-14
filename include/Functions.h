#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <sqlite3.h>
#include <string>
#include <vector>

bool executeSQL(sqlite3* db, const std::string& sql);

bool createPreventOverSaleTrigger(sqlite3* db);
void fillPeriodStats(sqlite3* db, const std::string& startDate, const std::string& endDate);
void showDiscSalesForPeriod(sqlite3* db, int disc_id, const std::string& startDate, const std::string& endDate);

std::vector<unsigned char> readImageFile(const std::string& filename);
bool saveImageToDisc(sqlite3* db, int disc_id, const std::string& imagePath);
bool loadImageFromDisc(sqlite3* db, int disc_id, const std::string& outputPath);
void showDiscsWithImages(sqlite3* db);

#endif