#include "Database.h"

ChatDB::ChatDB(const std::string& ChatDB_name) {
    if (sqlite3_open(ChatDB_name.c_str(), &db)) {
        throw std::runtime_error("Не получилось открыть базу: " + std::string(sqlite3_errmsg(db)));
    }
    const char* createUsers =
    "CREATE TABLE IF NOT EXISTS users ( \
    id INTEGER PRIMARY KEY AUTOINCREMENT, \
    login TEXT UNIQUE NOT NULL, \
    username TEXT, \
    password TEXT NOT NULL \
    );";

const char* createMessages =
    "CREATE TABLE IF NOT EXISTS messages ( \
    id INTEGER PRIMARY KEY AUTOINCREMENT, \
    user_id INTEGER, \
    receiver_id INTEGER, \
    message TEXT, \
    timestamp DATETIME, \
    FOREIGN KEY(user_id) REFERENCES users(id), \
    FOREIGN KEY(receiver_id) REFERENCES users(id) \
    )";

const char* createLogs =
    "CREATE TABLE IF NOT EXISTS logs ( \
    id INTEGER PRIMARY KEY AUTOINCREMENT, \
    action TEXT, \
    timestamp DATETIME \
    );";

sqlite3_exec(db, createUsers, nullptr, nullptr, nullptr);
sqlite3_exec(db, createMessages, nullptr, nullptr, nullptr);
sqlite3_exec(db, createLogs, nullptr, nullptr, nullptr);

logAction("Открыли базу " + ChatDB_name);
}


ChatDB::~ChatDB() {
    logAction("Закрыли базу");
    sqlite3_close(db);
}
/*int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for(int i = 0; i < argc; i++) {
        std::cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << std::endl;
    }
    std::cout << std::endl;
    return 0;
}*/

int ChatDB::getUserId(const std::string& login) {
    //char* errMsg;
    std::lock_guard<std::mutex> lock(ChatDB Mutex);

    sqlite3_stmt *stmt;
    int userId = -1;

    const char* sql = "SELECT id FROM users WHERE login = ?;";
    
    /*if (sqlite3_exec(ChatDB , sql.c_str(), callback, &userId, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Error " << errMsg << std::endl;
        sqlite3_free(errMsg); 
        return -1;
    } else {
        std::cout << "Все найс едем дальше" << std::endl;
    }
    */

    /*sqlite3_prepare_v2(
        sqlite3* ChatDB , - база данных
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
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }
    //std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC); 
    //std::cout << "Положили в строку искомое значение" << std::endl;

    if (sqlite3_step(stmt)/*выполение команды*/ == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    //std::cout << "Сделали поиск и привязку значения к переменной" << std::endl;

    sqlite3_finalize(stmt); //освободили ресурсы
    //std::cout << "освободили ресурсы" << std::endl;
    //std::cout << "значение переменной userID " << userId << std::endl;
    logAction("Получили id пользователя с ником " + login);
    return userId;
}

void ChatDB ::addMessage(const std::string& sender, const std::string& receiver, const std::string& content) {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    sqlite3_stmt *stmt;

    const char* sql = "INSERT INTO messages (user_id, receiver_id, message, timestamp) \
        VALUES ((SELECT id FROM users WHERE login = ?), \
                (SELECT id FROM users WHERE login = ?), ?, datetime('now'));";
    
    
    if (sqlite3_prepare_v2(db , sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);

    if (receiver.empty()) {
        sqlite3_bind_null(stmt, 2);
    } else {
        sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_STATIC);
    }
    
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        std::cout << "Сообщение успешно добавлено" << std::endl;
    } else {
        std::cerr << "Ошибка добавления сообщения: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    //std::cout << "освободили ресурсы" << std::endl;
    logAction("Добавили сообщение пользователей " + sender + " и" + receiver + ", текст сообщения " + content);
    return;
}

std::vector<std::string> ChatDB::getMessages(const std::string& user1, const std::string& user2) {
    if (user1.empty() || user2.empty()) return {};
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    std::vector<std::string> messages;
    sqlite3_stmt *stmt;

    const char* sql = "SELECT u.login as sender, m.message, m.timestamp \
                        FROM messages m \
                        JOIN users u ON u.id = m.user_id \
                        WHERE ( \
                            (m.user_id = (SELECT id FROM users WHERE login = ?) AND \
                            m.receiver_id = (SELECT id FROM users WHERE login = ?)) \
                            OR \
                            (m.user_id = (SELECT id FROM users WHERE login = ?) AND \
                            m.receiver_id = (SELECT id FROM users WHERE login = ?)) \
                        ) \
                        ORDER BY m.timestamp ASC";

    
    if (sqlite3_prepare_v2(db , sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }
    
    sqlite3_bind_text(stmt, 1, user1.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user2.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user2.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user1.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if (sender && message && timestamp) {
            messages.push_back(std::string ("[") + timestamp + "]" + sender + ": " + message);
        }
    }

    sqlite3_finalize(stmt);
    logAction("Получили список сообщений пользователей " + user1 + ", " + user2);
    return messages;
}

std::vector<std::string> ChatDB ::getPublicMessages() {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    std::vector<std::string> messages;
    sqlite3_stmt *stmt;

    const char* sql = "SELECT u.login as sender, m.message, m.timestamp \
                      FROM messages m \
                      JOIN users u ON u.id = m.user_id \
                      WHERE m.receiver_id IS NULL \
                      ORDER BY m.timestamp ASC";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if (sender && message && timestamp) {
            messages.push_back(std::string("[") + timestamp + "] " + sender + ": " + message);
        }
    }

    sqlite3_finalize(stmt);
    logAction("Получили историю общего чата");
    return messages;
}

void ChatDB ::addUser(const std::string& login, const std::string& name, const std::string& password) {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    sqlite3_stmt *stmt;

    std::string password_salt = "strong_salt_12345" + password;
    std::hash<std::string> hash;

    size_t hash_value = hash(password_salt); 
    std::string hash_password = std::to_string(hash_value);

    const char* sql = "INSERT INTO users (login, username, password) VALUES (?, ?, ?);";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    //std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;
    
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hash_password.c_str(), -1, SQLITE_STATIC);
    //std::cout << "Положили в строки искомое значение" << std::endl;

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Ошибка добавления пользователя: " << sqlite3_errmsg(db) << std::endl;
    } else {
        std::cout << "Пользователь успешно добавлен" << std::endl;
    }

    sqlite3_finalize(stmt);
    logAction("Добавили пользователя с логином " + login + " и именем " + name);
    //std::cout << "освободили ресурсы" << std::endl;
}

bool ChatDB ::verifyUser(const std::string& login, const std::string& password) {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    sqlite3_stmt *stmt;
    
    const char* sql = "SELECT password FROM users WHERE login = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    //std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    //std::cout << "Положили в строку искомое значение" << std::endl;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if (stored_hash) {
            std::string password_salt = "strong_salt_12345" + password;
            std::hash<std::string> hash;
            size_t hash_value = hash(password_salt); 
            std::string hash_password = std::to_string(hash_value);

            sqlite3_finalize(stmt);
            //std::cout << "освободили ресурсы" << std::endl;
            logAction("Проверили пользователя " + login);
            return hash_password == stored_hash;
        }
    }
    logAction("Не смогли проверить пользователя " + login);
    return false;
}

std::vector<std::string> ChatDB ::getUserMessages(const std::string& login) {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    sqlite3_stmt *stmt;
    std::vector<std::string> messages;

    const char* sql = "SELECT u.login, MAX(m.timestamp) \
                        FROM messages m \
                        JOIN users u ON u.id = m.receiver_id \
                        WHERE m.user_id = (SELECT id FROM users WHERE login = ?) \
                        \
                        UNION \
                        \
                        SELECT u.login, MAX(m.timestamp) \
                        FROM messages m \
                        JOIN users u ON u.id = m.user_id \
                        WHERE m.receiver_id = (SELECT id FROM users WHERE login = ?) \
                        \
                        ORDER BY 2 DESC \
                    ";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }
    //std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, login.c_str(), -1, SQLITE_STATIC);
    //std::cout << "Положили в строку искомое значение" << std::endl;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        if (message && timestamp) {
            messages.push_back(std::string(message) + " (" + std::string(timestamp ? timestamp : "нет сообщений") + ")");
        }
    }
    //std::cout << "Сделали поиск и привязку значения к переменной" << std::endl;

    sqlite3_finalize(stmt); //освободили ресурсы
    //std::cout << "освободили ресурсы" << std::endl;
    logAction("Вывели пользователю " + login + " сообщения");
    return messages;
}

std::vector<std::string> ChatDB ::getUserList() {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
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
        //std::cout << "освободили ресурсы" << std::endl;
        return loginUsers;
    } else {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }
    logAction("Получили список пользователей");
}

void ChatDB ::logAction(const std::string& action) {
    std::lock_guard<std::mutex> lock(ChatDB Mutex);
    sqlite3_stmt *stmt;
    const char* sql = "INSERT INTO logs (action, timestamp) VALUES (?, datetime('now'));";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки: " << std::endl;
        return;
    }
    //std::cout << "Выполнили подготовку строки с сырым значением" << std::endl;

    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
    //std::cout << "Положили в строку искомое значение" << std::endl;

    sqlite3_step(stmt);/*выполение команд*/
    sqlite3_finalize(stmt); //освободили ресурсы
    //std::cout << "освободили ресурсы" << std::endl;
    return;
}