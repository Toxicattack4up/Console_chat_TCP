
#include "DB.h"

DB::DB(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db)) {
        throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
    }
}

DB::~DB() {
    sqlite3_close(db);
}

/*int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for(int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << std::endl;
    return 0;
}*/

int DB::getUserId(const std::string& login) {
    //char* errMsg;
    std::lock_guard<std::mutex> lock(dbMutex);

    sqlite3_stmt *stmt;
    int userId = -1;

    const char* sql = "SELECT id FROM users WHERE login = ?;";
    
    /*if (sqlite3_exec(db, sql.c_str(), callback, &userId, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Error " << errMsg << std::endl;
        sqlite3_free(errMsg); 
        return -1;
    } else {
        std::cout << "Все найс едем дальше" << std::endl;
    }
    */

    /*sqlite3_prepare_v2(
        sqlite3* db, - база данных
        const char *zSql, - текст в виде строки char 
        int nByte, - длина передаваемой строки
        sqlite3_stmt **ppStmt, - указатель на подготовленное выражение stmt
        const char **pzTail); - указатель на остаток строки после SQL выражения

        Последовательность добавления команд 
        1. sqlite3_prepare_v2 - подготовка SQL выражения
        2. sqlite3_bind_text - привязка параметров
        3. sqlite3_step - выполнение подготовленного выражения
        4. sqlite3_column_int - получение значения столбца
        5. sqlite3_finalize - освобождение ресурсов
    */

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare" << std::endl;
        return -1;
    }
    std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC); 
    std::cout << "Положили в строку искомое значение" << std::endl;

    if (sqlite3_step(stmt)/*выполение команды*/ == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    std::cout << "Сделали поиск и привязку значения к переменной" << std::endl;

    sqlite3_finalize(stmt); //освободили ресурсы
    std::cout << "освободили ресурсы" << std::endl;
    std::cout << "значение переменной userID " << userId << std::endl;
    return userId;
}

void DB::addMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt *stmt;

    const char* sql = "INSERT INTO messages (user_id, receiver_id, message, timestamp) \
        VALUES ((SELECT id FROM users WHERE login = ?), \
                (SELECT id FROM users WHERE login = ?), ?, datetime('now'));";
    
    
    if (sqlite3_prepare_v2(db, receiver.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки первой части строки для базы" << sqlite3_errmsg(db) << std::endl;
        return;
    }

    if (receiver.empty()) {
        //sql = "INSERT INTO messages (user_id, receiver_id, message, timestamp) VALUES (?, (SELECT id FROM users WHERE login = ?), ?, datetime('now'));";
        sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_null(stmt, 2);
        sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    } else {
        //sql = "INSERT INTO messages (user_id, message, timestamp) VALUES ((SELECT id FROM users WHERE login = ?), ?, datetime('now'));";
        sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    }

    
    std::cout << "Выполнена подготовка строки для добавления сообщения" << std::endl;

    //sqlite3_bind_text(stmt, 1, sql, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        std::cout << "Сообщение успешно добавлено" << std::endl;
    } else {
        std::cerr << "Ошибка добавления сообщения: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    std::cout << "освободили ресурсы" << std::endl;
    return;
}

std::vector<std::string> DB::getMessages(const std::string& user1, const std::string& user2) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt *stmt;

    const char* sql = "INSERT INTO messages ()";

    return;
}

void DB::addUser(const std::string& login, const std::string& name, const std::string& password) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt *stmt;

    std::string password_salt = "strong_salt_12345" + password;
    std::hash<std::string> hash;

    size_t hash_value = hash(password_salt); 
    std::string hash_password = std::to_string(hash_value);

    const char* sql = "INSERT INTO users (login, username, password) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare" << sqlite3_errmsg(db) << std::endl;
        return;
    }
    std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;
    
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hash_password.c_str(), -1, SQLITE_STATIC);
    std::cout << "Положили в строки искомое значение" << std::endl;

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Ошибка добавления пользователя: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Пользователь успешно добавлен" << std::endl;
    }

    sqlite3_finalize(stmt);
    std::cout << "освободили ресурсы" << std::endl;
}

bool DB::verifyUser(const std::string& login, const std::string& password) {
    
    return false;
}

std::vector<std::string> DB::getUserMessages(const std::string& login) {

    return {};
}

std::vector<std::string> DB::getUserList() {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt *stmt;
    std::vector<std::string> loginUsers;
    const char* sql = "SELECT login FROM users";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* login = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));

            if (login) {
                loginUsers.push_back(login);
            }
        }
        
        sqlite3_finalize(stmt);
        std::cout << "освободили ресурсы" << std::endl;
        return loginUsers;
    } else {
        std::cerr << "ошибка подготовки строки" << sqlite3_errmsg(db) << std::endl;
        return {};
    }
}

void DB::logAction(const std::string& action) {
    std::lock_guard<std::mutex> lock(dbMutex);
    sqlite3_stmt *stmt;
    const char* sql = "INSERT INTO logs (action, timestamp) VALUES (?, datetime('now'));";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare" << std::endl;
        return;
    }
    std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;

    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
    std::cout << "Положили в строку искомое значение" << std::endl;

    sqlite3_step(stmt);/*выполение команд*/
    sqlite3_finalize(stmt); //освободили ресурсы
    std::cout << "освободили ресурсы" << std::endl;
    return;
}