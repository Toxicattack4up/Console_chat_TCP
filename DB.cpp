#pragma once
#include "DB.h"

DB::DB(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db)) {
        throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
    }
}

DB::~DB() {
    sqlite3_close(db);
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for(int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << std::endl;
    return 0;
}

int DB::getUserId(const std::string& login) {
    char* errMsg;
    std::lock_guard<std::mutex> lock(dbMutex);

    std::string sql = "SELECT id FROM users WHERE login = ?;";
    sqlite3_stmt *stmt;
    char text = sqlite3_exec(db, sql.c_str(), callback, 0, &errMsg);
    if (text != SQLITE_OK)
    {
        std::cerr << "Error " << errMsg << std::endl;
        sqlite3_free(errMsg); 
    } else {
        std::cout << "Все найс едем дальше" << std::endl;
    }
    
    //sqlite3_prepare_v2(sqlite3*, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
    /*if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    int userId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);*/
}

void DB::addMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    std::lock_guard<std::mutex> lock(dbMutex);

    std::string sql = "INSERT INTO messages (user_id, receiver_id, message, timestamp) VALUES (?, ?, ?, datetime('now'));";
    sqlite3_stmt *stmt;
    
    if (!receiver.empty()) {
        sql = "INSERT INTO messages (user_id, receiver_id, message, timestamp) VALUES (?, (SELECT id FROM users WHERE login = ?), ?, datetime('now'));";
    } else {
        sql = "INSERT INTO messages (user_id, message, timestamp) VALUES ((SELECT id FROM users WHERE login = ?), ?, datetime('now'));";
    }
}

std::vector<std::string> DB::getMessages(const std::string& user1, const std::string& user2) {

}

void DB::addUser(const std::string& login, const std::string& password, const std::string& name) {

}

bool DB::verifyUser(const std::string& login, const std::string& password) {
    
    return false;
}

std::vector<std::string> DB::getUserMessages(const std::string& login) {

}

std::vector<std::string> DB::getUserList() {
    return {};
}

void DB::logAction(const std::string& action) {

}