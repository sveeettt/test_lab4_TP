#include "Functions.h"
#include <iostream>
#include <fstream>
#include <vector>

bool executeSQL(sqlite3* db, const std::string& sql) {
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool createPreventOverSaleTrigger(sqlite3* db) {
    const char* sql = R"(
        CREATE TRIGGER IF NOT EXISTS prevent_over_sale
        BEFORE INSERT ON "TRANSACTION"
        WHEN NEW.operation_type = 'sale'
        BEGIN
            SELECT CASE
                WHEN (
                    (SELECT SUM(CASE WHEN operation_type = 'income' THEN quantity ELSE 0 END) FROM "TRANSACTION" WHERE disc_id = NEW.disc_id) -
                    (SELECT SUM(CASE WHEN operation_type = 'sale' THEN quantity ELSE 0 END) FROM "TRANSACTION" WHERE disc_id = NEW.disc_id)
                ) < NEW.quantity
                THEN RAISE(ABORT, 'ERROR: Not enough discs in stock!')
            END;
        END;
    )";

    if (executeSQL(db, sql)) {
        std::cout << "Trigger created" << std::endl;
        return true;
    }
    return false;
}

void fillPeriodStats(sqlite3* db, const std::string& startDate, const std::string& endDate) {
    std::string sqlDelete = "DELETE FROM period_stats WHERE period_start = '" + startDate + "' AND period_end = '" + endDate + "';";
    executeSQL(db, sqlDelete);

    std::string sqlInsert =
        "INSERT INTO period_stats (period_start, period_end, disc_id, total_income, total_sold) "
        "SELECT "
        "   '" + startDate + "', "
        "   '" + endDate + "', "
        "   disc_id, "
        "   SUM(CASE WHEN operation_type = 'income' THEN quantity ELSE 0 END) as total_income, "
        "   SUM(CASE WHEN operation_type = 'sale' THEN quantity ELSE 0 END) as total_sold "
        "FROM \"TRANSACTION\" "
        "WHERE operation_date BETWEEN '" + startDate + "' AND '" + endDate + "' "
        "GROUP BY disc_id;";

    if (executeSQL(db, sqlInsert)) {
        std::cout << "\nPeriod statistics saved" << std::endl;

        std::string sqlSelect =
            "SELECT disc_id, total_income, total_sold FROM period_stats "
            "WHERE period_start = '" + startDate + "' AND period_end = '" + endDate + "';";
        executeSQL(db, sqlSelect);
    }
}

void showDiscSalesForPeriod(sqlite3* db, int disc_id, const std::string& startDate, const std::string& endDate) {
    std::string sql =
        "SELECT "
        "   operation_date, "
        "   quantity, "
        "   (quantity * cd.price) as total_price "
        "FROM TRANSACTION t "
        "JOIN CD_DISC cd ON t.disc_id = cd.disc_id "
        "WHERE t.disc_id = " + std::to_string(disc_id) +
        "   AND t.operation_type = 'sale' "
        "   AND t.operation_date BETWEEN '" + startDate + "' AND '" + endDate + "' "
        "ORDER BY operation_date;";

    std::cout << "\n=== DISC " << disc_id << " SALES (" << startDate << " - " << endDate << ") ===" << std::endl;

    sqlite3_stmt* stmt;
    int total_qty = 0;
    double total_sum = 0;
    int count = 0;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* date = (const char*)sqlite3_column_text(stmt, 0);
            int qty = sqlite3_column_int(stmt, 1);
            double price = sqlite3_column_double(stmt, 2);

            std::cout << "  Date: " << date << std::endl;
            std::cout << "  Quantity: " << qty << std::endl;
            std::cout << "  Total: " << price << std::endl;
            std::cout << "------------------------" << std::endl;

            total_qty += qty;
            total_sum += price;
            count++;
        }
        sqlite3_finalize(stmt);
    }

    if (count > 0) {
        std::cout << "\nTOTAL:" << std::endl;
        std::cout << "  Total quantity: " << total_qty << std::endl;
        std::cout << "  Total amount: " << total_sum << std::endl;
    }
    else {
        std::cout << "No sales found for this period" << std::endl;
    }
}

std::vector<unsigned char> readImageFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

bool saveImageToDisc(sqlite3* db, int disc_id, const std::string& imagePath) {
    std::vector<unsigned char> imageData = readImageFile(imagePath);
    if (imageData.empty()) {
        return false;
    }

    std::string sql = "UPDATE CD_DISC SET cover_image = ? WHERE disc_id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_blob(stmt, 1, imageData.data(), imageData.size(), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, disc_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        std::cout << "Image saved for disc " << disc_id << std::endl;
        return true;
    }

    std::cerr << "Save error: " << sqlite3_errmsg(db) << std::endl;
    return false;
}

bool loadImageFromDisc(sqlite3* db, int disc_id, const std::string& outputPath) {
    std::string sql = "SELECT cover_image FROM CD_DISC WHERE disc_id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, disc_id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        std::cout << "No image found for disc " << disc_id << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    const void* blob = sqlite3_column_blob(stmt, 0);
    int size = sqlite3_column_bytes(stmt, 0);

    if (blob == nullptr || size == 0) {
        std::cout << "Image data is empty for disc " << disc_id << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot create output file: " << outputPath << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    file.write(static_cast<const char*>(blob), size);
    file.close();
    sqlite3_finalize(stmt);

    std::cout << "Image saved to: " << outputPath << std::endl;
    return true;
}

void showDiscsWithImages(sqlite3* db) {
    const char* sql = "SELECT disc_id, manufacturer, price, length(cover_image) as img_size FROM CD_DISC WHERE cover_image IS NOT NULL;";
    
    std::cout << "\n=== DISCS WITH IMAGES ===" << std::endl;
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int disc_id = sqlite3_column_int(stmt, 0);
            const char* manufacturer = (const char*)sqlite3_column_text(stmt, 1);
            double price = sqlite3_column_double(stmt, 2);
            int img_size = sqlite3_column_int(stmt, 3);
            
            std::cout << "  disc_id: " << disc_id << std::endl;
            std::cout << "  manufacturer: " << manufacturer << std::endl;
            std::cout << "  price: " << price << std::endl;
            std::cout << "  image_size: " << img_size << " bytes" << std::endl;
            std::cout << "------------------------" << std::endl;
        }
        sqlite3_finalize(stmt);
    }
}