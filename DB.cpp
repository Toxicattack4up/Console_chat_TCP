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

void DB::addMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    
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