#include "Queries.h"
#include <iostream>
#include <string>

int callback(void* data, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        std::cout << "  " << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << "------------------------" << std::endl;
    return 0;
}

void showSoldAndRemaining(sqlite3* db) {
    const char* sql = R"(
        SELECT 
            cd.disc_id,
            cd.manufacturer,
            cd.price,
            s.total_income as total_received,
            s.total_sold as total_sold,
            s.remaining as remaining
        FROM CD_DISC cd
        LEFT JOIN STOCK s ON cd.disc_id = s.disc_id
        ORDER BY (s.total_sold - s.remaining) DESC;
    )";

    std::cout << "\n=== SOLD AND REMAINING DISCS ===" << std::endl;
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, callback, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void showSalesByDiscAndPeriod(sqlite3* db, int disc_id, const std::string& startDate, const std::string& endDate) {
    std::string sql =
        "SELECT "
        "   t.operation_date, "
        "   t.quantity, "
        "   (t.quantity * cd.price) as total_cost "
        "FROM \"transaction\" t "
        "JOIN cd_disc cd ON t.disc_id = cd.disc_id "
        "WHERE t.disc_id = " + std::to_string(disc_id) +
        "   AND t.operation_type = 'sale' "
        "   AND t.operation_date BETWEEN '" + startDate + "' AND '" + endDate + "' "
        "ORDER BY t.operation_date;";

    std::cout << "\n=== SALES FOR DISC " << disc_id << " (" << startDate << " - " << endDate << ") ===" << std::endl;
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void showMostSoldDiscDetails(sqlite3* db) {
    const char* sqlFind = R"(
        SELECT disc_id, SUM(quantity) as total_sold
        FROM "transaction"
        WHERE operation_type = 'sale'
        GROUP BY disc_id
        ORDER BY total_sold DESC
        LIMIT 1;
    )";

    sqlite3_stmt* stmt;
    int most_sold_id = -1;
    int max_sold = 0;

    if (sqlite3_prepare_v2(db, sqlFind, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            most_sold_id = sqlite3_column_int(stmt, 0);
            max_sold = sqlite3_column_int(stmt, 1);
        }
        sqlite3_finalize(stmt);
    }

    if (most_sold_id != -1) {
        std::cout << "\n=== MOST POPULAR DISC ===" << std::endl;

        std::string sqlDisc = "SELECT disc_id, manufacture_date, manufacturer, price FROM cd_disc WHERE disc_id = " +
            std::to_string(most_sold_id) + ";";
        sqlite3_exec(db, sqlDisc.c_str(), callback, nullptr, nullptr);

        std::cout << "\n=== TRACKS ON THIS DISC ===" << std::endl;
        std::string sqlTracks = "SELECT title, author, performer FROM track WHERE disc_id = " +
            std::to_string(most_sold_id) + ";";
        sqlite3_exec(db, sqlTracks.c_str(), callback, nullptr, nullptr);
    }
}

void showTopPerformerSales(sqlite3* db) {
    const char* sql = R"(
        SELECT 
            t.performer,
            SUM(tr.quantity) as total_sold
        FROM track t
        JOIN "transaction" tr ON t.disc_id = tr.disc_id
        WHERE tr.operation_type = 'sale'
        GROUP BY t.performer
        ORDER BY total_sold DESC
        LIMIT 1;
    )";

    std::cout << "\n=== TOP PERFORMER ===" << std::endl;
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, callback, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void showAuthorStats(sqlite3* db) {
    const char* sql = R"(
        SELECT 
            t.author,
            SUM(tr.quantity) as total_sold,
            SUM(tr.quantity * cd.price) as total_amount
        FROM track t
        JOIN cd_disc cd ON t.disc_id = cd.disc_id
        JOIN "transaction" tr ON cd.disc_id = tr.disc_id
        WHERE tr.operation_type = 'sale'
        GROUP BY t.author
        ORDER BY total_amount DESC;
    )";

    std::cout << "\n=== AUTHOR STATISTICS ===" << std::endl;
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, callback, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}