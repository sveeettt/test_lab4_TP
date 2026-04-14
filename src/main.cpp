#include <iostream>
#include <string>
#include <limits>
#include "Database.h"
#include "Auth.h"
#include "Queries.h"
#include "Functions.h"

using namespace std;

void showMenu(bool isAdmin) {
    cout << "\n========== MUSIC SALON ==========" << endl;
    cout << "1. Show sold and remaining discs" << endl;
    cout << "2. Show disc sales by period" << endl;
    cout << "3. Show most popular disc" << endl;
    cout << "4. Show top performer" << endl;
    cout << "5. Show author statistics" << endl;
    cout << "6. Fill period statistics (function)" << endl;
    cout << "7. Show disc sales for period (function)" << endl;

    if (isAdmin) {
        cout << "\n--- ADMIN FUNCTIONS ---" << endl;
        cout << "8. Add new disc" << endl;
        cout << "9. Add operation (income/sale)" << endl;
        cout << "10. Update disc price" << endl;
        cout << "11. Delete disc" << endl;
    }

    cout << "0. Exit" << endl;
    cout << "===================================" << endl;
    cout << "Choice: ";
}

void addNewCD(sqlite3* db) {
    int disc_id;
    string date, manufacturer;
    double price;

    cout << "\n--- ADD NEW DISC ---" << endl;
    cout << "Enter disc ID: ";
    cin >> disc_id;
    cin.ignore();
    cout << "Enter manufacture date (YYYY-MM-DD): ";
    getline(cin, date);
    cout << "Enter manufacturer: ";
    getline(cin, manufacturer);
    cout << "Enter price: ";
    cin >> price;

    string sql = "INSERT INTO CD_DISC (disc_id, manufacture_date, manufacturer, price) VALUES (" +
        to_string(disc_id) + ", '" + date + "', '" + manufacturer + "', " + to_string(price) + ");";

    if (executeSQL(db, sql)) {
        cout << "Disc added successfully!" << endl;
    }
}

void addTransaction(sqlite3* db) {
    int disc_id, quantity;
    string date, type;

    cout << "\n--- ADD OPERATION ---" << endl;
    cout << "Enter disc ID: ";
    cin >> disc_id;
    cin.ignore();
    cout << "Enter date (YYYY-MM-DD): ";
    getline(cin, date);
    cout << "Enter operation type (income/sale): ";
    getline(cin, type);
    cout << "Enter quantity: ";
    cin >> quantity;

    string sql = "INSERT INTO \"TRANSACTION\" (operation_date, operation_type, disc_id, quantity) VALUES ('" +
        date + "', '" + type + "', " + to_string(disc_id) + ", " + to_string(quantity) + ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cout << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    else {
        cout << "Operation added!" << endl;
        Database dbObj("");
        dbObj.updateSTOCKFromTRANSACTIONs();
    }
}

int main() {
    string db_path;
    cout << "Enter database file path: ";
    cin >> db_path;

    Database db(db_path);
    if (!db.connect()) {
        cerr << "Failed to connect to database!" << endl;
        return 1;
    }

    createPreventOverSaleTrigger(db.getDB());

    string username, password;
    cout << "\n=== AUTHENTICATION ===" << endl;
    cout << "Login: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    User currentUser;
    if (!authenticate(db.getDB(), username, password, currentUser)) {
        cout << "Authentication failed! Invalid login or password." << endl;
        return 1;
    }

    cout << "\nWelcome, " << currentUser.username << "!" << endl;
    cout << "Your role: " << currentUser.role << endl;

    bool isAdmin = (currentUser.role == "admin");
    int choice;

    do {
        showMenu(isAdmin);
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1:
            showSoldAndRemaining(db.getDB());
            break;

        case 2: {
            int disc_id;
            string start, end;
            cout << "Enter disc ID: ";
            cin >> disc_id;
            cout << "Enter start date (YYYY-MM-DD): ";
            cin >> start;
            cout << "Enter end date (YYYY-MM-DD): ";
            cin >> end;
            showSalesByDiscAndPeriod(db.getDB(), disc_id, start, end);
            break;
        }

        case 3:
            showMostSoldDiscDetails(db.getDB());
            break;

        case 4:
            showTopPerformerSales(db.getDB());
            break;

        case 5:
            showAuthorStats(db.getDB());
            break;

        case 6: {
            string start, end;
            cout << "Enter start date (YYYY-MM-DD): ";
            cin >> start;
            cout << "Enter end date (YYYY-MM-DD): ";
            cin >> end;
            fillPeriodStats(db.getDB(), start, end);
            break;
        }

        case 7: {
            int disc_id;
            string start, end;
            cout << "Enter disc ID: ";
            cin >> disc_id;
            cout << "Enter start date (YYYY-MM-DD): ";
            cin >> start;
            cout << "Enter end date (YYYY-MM-DD): ";
            cin >> end;
            showDiscSalesForPeriod(db.getDB(), disc_id, start, end);
            break;
        }

        case 8:
            if (isAdmin) addNewCD(db.getDB());
            else cout << "Access denied!" << endl;
            break;

        case 9:
            if (isAdmin) addTransaction(db.getDB());
            else cout << "Access denied!" << endl;
            break;

        case 10: {
            if (isAdmin) {
                int disc_id;
                double new_price;
                cout << "Enter disc ID to update price: ";
                cin >> disc_id;
                cout << "Enter new price: ";
                cin >> new_price;
                string sql = "UPDATE CD_DISC SET price = " + to_string(new_price) + " WHERE disc_id = " + to_string(disc_id) + ";";
                if (executeSQL(db.getDB(), sql)) {
                    cout << "Price updated!" << endl;
                }
            }
            else {
                cout << "Access denied!" << endl;
            }
            break;
        }

        case 11:
            if (isAdmin) {
                int disc_id;
                cout << "Enter disc ID to delete: ";
                cin >> disc_id;
                string sql = "DELETE FROM CD_DISC WHERE disc_id = " + to_string(disc_id) + ";";
                if (executeSQL(db.getDB(), sql)) {
                    cout << "Disc deleted!" << endl;
                    Database dbObj("");
                    dbObj.updateSTOCKFromTRANSACTIONs();
                }
            }
            else {
                cout << "Access denied!" << endl;
            }
            break;

        case 0:
            cout << "Goodbye!" << endl;
            break;

        default:
            cout << "Invalid choice!" << endl;
        }

    } while (choice != 0);

    return 0;
}