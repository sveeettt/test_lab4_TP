#include <gtest/gtest.h>
#include <sqlite3.h>
#include "Auth.h"
#include "Database.h"
#include "Functions.h"
#include "Queries.h"

// ============================================================
// ТЕСТЫ ДЛЯ АУТЕНТИФИКАЦИИ (Auth) - 9 тестов
// ============================================================

class AuthTests : public ::testing::Test {
protected:
    sqlite3* db;

    void SetUp() override {
        sqlite3_open(":memory:", &db);
        const char* sql = R"(
            CREATE TABLE users (
                id INTEGER PRIMARY KEY,
                username TEXT,
                password TEXT,
                role TEXT
            );
            INSERT INTO users VALUES (1, 'admin', 'admin123', 'admin');
            INSERT INTO users VALUES (2, 'user1', 'pass123', 'user');
            INSERT INTO users VALUES (3, 'manager', 'mgr456', 'manager');
        )";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }

    void TearDown() override {
        sqlite3_close(db);
    }
};

TEST_F(AuthTests, ValidAdminLogin) {
    User user;
    bool result = authenticate(db, "admin", "admin123", user);
    ASSERT_TRUE(result);
    EXPECT_EQ(user.username, "admin");
    EXPECT_EQ(user.role, "admin");
}

TEST_F(AuthTests, ValidUserLogin) {
    User user;
    bool result = authenticate(db, "user1", "pass123", user);
    ASSERT_TRUE(result);
    EXPECT_EQ(user.username, "user1");
    EXPECT_EQ(user.role, "user");
}

TEST_F(AuthTests, ValidManagerLogin) {
    User user;
    bool result = authenticate(db, "manager", "mgr456", user);
    ASSERT_TRUE(result);
    EXPECT_EQ(user.username, "manager");
    EXPECT_EQ(user.role, "manager");
}

TEST_F(AuthTests, InvalidPassword) {
    User user;
    bool result = authenticate(db, "admin", "wrong", user);
    ASSERT_FALSE(result);
}

TEST_F(AuthTests, EmptyPassword) {
    User user;
    bool result = authenticate(db, "admin", "", user);
    ASSERT_FALSE(result);
}

TEST_F(AuthTests, EmptyUsername) {
    User user;
    bool result = authenticate(db, "", "admin123", user);
    ASSERT_FALSE(result);
}

TEST_F(AuthTests, NonExistentUser) {
    User user;
    bool result = authenticate(db, "fakeuser", "pass", user);
    ASSERT_FALSE(result);
}

TEST_F(AuthTests, CaseSensitiveUsername) {
    User user;
    bool result = authenticate(db, "ADMIN", "admin123", user);
    ASSERT_FALSE(result);
}

TEST_F(AuthTests, CaseSensitivePassword) {
    User user;
    bool result = authenticate(db, "admin", "ADMIN123", user);
    ASSERT_FALSE(result);
}

// ============================================================
// ТЕСТЫ ДЛЯ АВТОРИЗАЦИИ (Authorize) - 6 тестов
// ============================================================

class AuthorizeTests : public ::testing::Test {
protected:
    User admin, user, manager;
    
    void SetUp() override {
        admin.role = "admin";
        user.role = "user";
        manager.role = "manager";
    }
};

TEST_F(AuthorizeTests, AdminCanAccessAdminFunctions) {
    EXPECT_TRUE(authorize(admin, "admin"));
}

TEST_F(AuthorizeTests, AdminCanAccessUserFunctions) {
    EXPECT_TRUE(authorize(admin, "user"));
}

TEST_F(AuthorizeTests, UserCannotAccessAdminFunctions) {
    EXPECT_FALSE(authorize(user, "admin"));
}

TEST_F(AuthorizeTests, UserCanAccessUserFunctions) {
    EXPECT_TRUE(authorize(user, "user"));
}

TEST_F(AuthorizeTests, ManagerCannotAccessAdmin) {
    EXPECT_FALSE(authorize(manager, "admin"));
}

TEST_F(AuthorizeTests, ManagerCannotAccessUser) {
    EXPECT_FALSE(authorize(manager, "user"));
}

// ============================================================
// ТЕСТЫ ДЛЯ БАЗЫ ДАННЫХ (Database) - 4 теста
// ============================================================

class DatabaseTests : public ::testing::Test {
protected:
    Database* db;
    
    void SetUp() override {
        db = new Database(":memory:");
        db->connect();
    }
    
    void TearDown() override {
        delete db;
    }
};

TEST_F(DatabaseTests, ConnectionSuccess) {
    EXPECT_TRUE(db->isConnected());
}

TEST_F(DatabaseTests, GetDBReturnsNonNull) {
    EXPECT_NE(db->getDB(), nullptr);
}

TEST_F(DatabaseTests, UpdateStockExecutesWithoutError) {
    EXPECT_NO_THROW(db->updateSTOCKFromTRANSACTIONs());
}

// ============================================================
// ТЕСТЫ ДЛЯ ФУНКЦИЙ (Functions) - 5 тестов
// ============================================================

class FunctionsTests : public ::testing::Test {
protected:
    sqlite3* db;
    
    void SetUp() override {
        sqlite3_open(":memory:", &db);
        // Создаём тестовые таблицы
        const char* sql = R"(
            CREATE TABLE CD_DISC (disc_id INTEGER PRIMARY KEY, price REAL);
            CREATE TABLE TRANSACTION (
                trans_id INTEGER PRIMARY KEY,
                operation_date TEXT,
                operation_type TEXT,
                disc_id INTEGER,
                quantity INTEGER
            );
            CREATE TABLE STOCK (disc_id INTEGER, total_income INTEGER, total_sold INTEGER, remaining INTEGER);
            CREATE TABLE period_stats (id INTEGER PRIMARY KEY, period_start TEXT, period_end TEXT, disc_id INTEGER, total_income INTEGER, total_sold INTEGER);
            
            INSERT INTO CD_DISC VALUES (1, 19.99);
            INSERT INTO TRANSACTION VALUES (1, '2024-01-01', 'income', 1, 100);
            INSERT INTO TRANSACTION VALUES (2, '2024-02-01', 'sale', 1, 30);
        )";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
    
    void TearDown() override {
        sqlite3_close(db);
    }
};

TEST_F(FunctionsTests, ExecuteSQLValid) {
    bool result = executeSQL(db, "SELECT * FROM CD_DISC");
    EXPECT_TRUE(result);
}

TEST_F(FunctionsTests, ExecuteSQLInvalid) {
    bool result = executeSQL(db, "SELECT * FROM non_existent_table");
    EXPECT_FALSE(result);
}

TEST_F(FunctionsTests, CreateTrigger) {
    bool result = createPreventOverSaleTrigger(db);
    EXPECT_TRUE(result);
}

TEST_F(FunctionsTests, FillPeriodStats) {
    EXPECT_NO_THROW(fillPeriodStats(db, "2024-01-01", "2024-12-31"));
}

TEST_F(FunctionsTests, ShowDiscSalesForPeriod) {
    EXPECT_NO_THROW(showDiscSalesForPeriod(db, 1, "2024-01-01", "2024-12-31"));
}

// ============================================================
// ТЕСТЫ ДЛЯ ЗАПРОСОВ (Queries) - 6 тестов
// ============================================================

class QueriesTests : public ::testing::Test {
protected:
    sqlite3* db;
    
    void SetUp() override {
        sqlite3_open(":memory:", &db);
        const char* sql = R"(
            CREATE TABLE CD_DISC (disc_id INTEGER PRIMARY KEY, manufacturer TEXT, price REAL);
            CREATE TABLE TRACK (track_id INTEGER PRIMARY KEY, title TEXT, author TEXT, performer TEXT, disc_id INTEGER);
            CREATE TABLE TRANSACTION (trans_id INTEGER PRIMARY KEY, operation_date TEXT, operation_type TEXT, disc_id INTEGER, quantity INTEGER);
            CREATE TABLE STOCK (disc_id INTEGER, total_income INTEGER, total_sold INTEGER, remaining INTEGER);
            
            INSERT INTO CD_DISC VALUES (1, 'Sony', 19.99);
            INSERT INTO CD_DISC VALUES (2, 'Universal', 24.99);
            INSERT INTO TRACK VALUES (1, 'Song1', 'Author1', 'Performer1', 1);
            INSERT INTO TRANSACTION VALUES (1, '2024-01-01', 'income', 1, 100);
            INSERT INTO TRANSACTION VALUES (2, '2024-02-01', 'sale', 1, 30);
            INSERT INTO STOCK VALUES (1, 100, 30, 70);
        )";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
    
    void TearDown() override {
        sqlite3_close(db);
    }
};

TEST_F(QueriesTests, ShowSoldAndRemainingNoCrash) {
    EXPECT_NO_THROW(showSoldAndRemaining(db));
}

TEST_F(QueriesTests, ShowSalesByDiscAndPeriodNoCrash) {
    EXPECT_NO_THROW(showSalesByDiscAndPeriod(db, 1, "2024-01-01", "2024-12-31"));
}

TEST_F(QueriesTests, ShowMostSoldDiscDetailsNoCrash) {
    EXPECT_NO_THROW(showMostSoldDiscDetails(db));
}

TEST_F(QueriesTests, ShowTopPerformerSalesNoCrash) {
    EXPECT_NO_THROW(showTopPerformerSales(db));
}

TEST_F(QueriesTests, ShowAuthorStatsNoCrash) {
    EXPECT_NO_THROW(showAuthorStats(db));
}

TEST_F(QueriesTests, CallbackFunctionWorks) {
    char* data = nullptr;
    char* argv[] = {"col1", "val1"};
    char* colName[] = {"column1"};
    int result = callback(data, 1, argv, colName);
    EXPECT_EQ(result, 0);
}

// ============================================================
// ТЕСТЫ ДЛЯ CRUD ОПЕРАЦИЙ - 4 теста
// ============================================================

class CRUDTests : public ::testing::Test {
protected:
    sqlite3* db;
    
    void SetUp() override {
        sqlite3_open(":memory:", &db);
        const char* sql = R"(
            CREATE TABLE CD_DISC (disc_id INTEGER PRIMARY KEY, manufacturer TEXT, price REAL);
            INSERT INTO CD_DISC VALUES (1, 'Sony', 19.99);
        )";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
    }
    
    void TearDown() override {
        sqlite3_close(db);
    }
};

TEST_F(CRUDTests, InsertNewDisc) {
    std::string sql = "INSERT INTO CD_DISC VALUES (99, 'Test', 9.99)";
    bool result = executeSQL(db, sql);
    EXPECT_TRUE(result);
}

TEST_F(CRUDTests, UpdateDiscPrice) {
    std::string sql = "UPDATE CD_DISC SET price = 49.99 WHERE disc_id = 1";
    bool result = executeSQL(db, sql);
    EXPECT_TRUE(result);
}

TEST_F(CRUDTests, DeleteDisc) {
    std::string sql = "DELETE FROM CD_DISC WHERE disc_id = 1";
    bool result = executeSQL(db, sql);
    EXPECT_TRUE(result);
}

TEST_F(CRUDTests, SelectDisc) {
    std::string sql = "SELECT * FROM CD_DISC WHERE disc_id = 1";
    bool result = executeSQL(db, sql);
    EXPECT_TRUE(result);
}

// ============================================================
// ТЕСТЫ ДЛЯ ТРИГГЕРА - 2 теста
// ============================================================

class TriggerTests : public ::testing::Test {
protected:
    sqlite3* db;
    
    void SetUp() override {
        sqlite3_open(":memory:", &db);
        const char* sql = R"(
            CREATE TABLE TRANSACTION (trans_id INTEGER PRIMARY KEY, operation_type TEXT, disc_id INTEGER, quantity INTEGER);
            CREATE TABLE STOCK (disc_id INTEGER, remaining INTEGER);
            INSERT INTO STOCK VALUES (1, 50);
        )";
        sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
        createPreventOverSaleTrigger(db);
    }
    
    void TearDown() override {
        sqlite3_close(db);
    }
};

TEST_F(TriggerTests, AllowsSaleWhenStockSufficient) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, "INSERT INTO TRANSACTION (operation_type, disc_id, quantity) VALUES ('sale', 1, 30)", nullptr, nullptr, &errMsg);
    EXPECT_EQ(rc, SQLITE_OK);
    sqlite3_free(errMsg);
}

TEST_F(TriggerTests, PreventsSaleWhenStockInsufficient) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, "INSERT INTO TRANSACTION (operation_type, disc_id, quantity) VALUES ('sale', 1, 100)", nullptr, nullptr, &errMsg);
    EXPECT_NE(rc, SQLITE_OK);
    sqlite3_free(errMsg);
}

// ============================================================
// ТОЧКА ВХОДА
// ============================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}