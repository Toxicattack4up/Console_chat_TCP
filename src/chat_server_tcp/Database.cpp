#include "Database.h"
#include <sodium.h>

namespace
{
bool ensureSodium()
{
    static const bool ok = (sodium_init() >= 0);
    return ok;
}

std::string hashPassword(const std::string &password)
{
    if (!ensureSodium())
    {
        throw std::runtime_error("libsodium init failed");
    }

    char hashed[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(hashed, password.c_str(), password.size(),
                          crypto_pwhash_OPSLIMIT_INTERACTIVE,
                          crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        throw std::runtime_error("password hashing failed");
    }
    return std::string(hashed);
}

bool verifyPassword(const std::string &password, const std::string &stored)
{
    if (!ensureSodium())
    {
        return false;
    }
    // libsodium hashes start with "$argon2"; legacy demo hashes were decimal std::hash
    if (stored.rfind("$argon2", 0) != 0)
    {
        return false;
    }
    return crypto_pwhash_str_verify(stored.c_str(), password.c_str(), password.size()) == 0;
}
} // namespace

ChatDB::ChatDB(const std::string &ChatDB_name)
{
    if (sqlite3_open(ChatDB_name.c_str(), &db))
    {
        throw std::runtime_error("Не получилось открыть базу: " + std::string(sqlite3_errmsg(db)));
    }
    const char *createUsers =
        "CREATE TABLE IF NOT EXISTS users ( \
    id INTEGER PRIMARY KEY AUTOINCREMENT, \
    login TEXT UNIQUE NOT NULL, \
    username TEXT, \
    password TEXT NOT NULL \
    );";

    const char *createMessages =
        "CREATE TABLE IF NOT EXISTS messages ( \
    id INTEGER PRIMARY KEY AUTOINCREMENT, \
    user_id INTEGER, \
    receiver_id INTEGER, \
    message TEXT, \
    timestamp DATETIME, \
    FOREIGN KEY(user_id) REFERENCES users(id), \
    FOREIGN KEY(receiver_id) REFERENCES users(id) \
    )";

    const char *createLogs =
        "CREATE TABLE IF NOT EXISTS logs ( \
    id INTEGER PRIMARY KEY AUTOINCREMENT, \
    action TEXT, \
    timestamp DATETIME \
    );";

    sqlite3_exec(db, createUsers, nullptr, nullptr, nullptr);
    sqlite3_exec(db, createMessages, nullptr, nullptr, nullptr);
    sqlite3_exec(db, createLogs, nullptr, nullptr, nullptr);

    logAction("Opened database " + ChatDB_name);
}

ChatDB::~ChatDB()
{
    if (db)
    {
        logAction("Closed database");
        sqlite3_close(db);
        db = nullptr;
    }
}

int ChatDB::getUserId(const std::string &login)
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;
    int userId = -1;

    const char *sql = "SELECT id FROM users WHERE login = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }
    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        userId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return userId;
}

void ChatDB::addMessage(const std::string &sender, const std::string &receiver, const std::string &content)
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;

    const char *sql = "INSERT INTO messages (user_id, receiver_id, message, timestamp) \
        VALUES ((SELECT id FROM users WHERE login = ?), \
                (SELECT id FROM users WHERE login = ?), ?, datetime('now'));";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, sender.c_str(), -1, SQLITE_STATIC);

    if (receiver.empty())
    {
        sqlite3_bind_null(stmt, 2);
    }
    else
    {
        sqlite3_bind_text(stmt, 2, receiver.c_str(), -1, SQLITE_STATIC);
    }

    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        std::cerr << "Ошибка добавления сообщения: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    // Do not log message body (privacy).
    logAction("Added message from " + sender + (receiver.empty() ? " (public)" : (" to " + receiver)));
}

std::vector<std::string> ChatDB::getMessages(const std::string &user1, const std::string &user2)
{
    if (user1.empty() || user2.empty())
        return {};
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    std::vector<std::string> messages;
    sqlite3_stmt *stmt;

    const char *sql = "SELECT u.login as sender, m.message, m.timestamp \
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

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    sqlite3_bind_text(stmt, 1, user1.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user2.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, user2.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, user1.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *sender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

        if (sender && message && timestamp)
        {
            messages.push_back(std::string("[") + timestamp + "]" + sender + ": " + message);
        }
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<std::string> ChatDB::getPublicMessages()
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    std::vector<std::string> messages;
    sqlite3_stmt *stmt;

    const char *sql = "SELECT u.login as sender, m.message, m.timestamp \
                      FROM messages m \
                      JOIN users u ON u.id = m.user_id \
                      WHERE m.receiver_id IS NULL \
                      ORDER BY m.timestamp ASC";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *sender = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));

        if (sender && message && timestamp)
        {
            messages.push_back(std::string("[") + timestamp + "] " + sender + ": " + message);
        }
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool ChatDB::addUser(const std::string &login, const std::string &name, const std::string &password)
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;

    std::string hash_password;
    try
    {
        hash_password = hashPassword(password);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Ошибка хеширования пароля: " << ex.what() << std::endl;
        return false;
    }

    const char *sql = "INSERT INTO users (login, username, password) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, hash_password.c_str(), -1, SQLITE_STATIC);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (!ok)
    {
        std::cerr << "Ошибка добавления пользователя: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    if (ok)
    {
        logAction("Added user " + login);
    }
    return ok;
}

bool ChatDB::verifyUser(const std::string &login, const std::string &password)
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;

    const char *sql = "SELECT password FROM users WHERE login = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *stored_hash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (stored_hash)
        {
            ok = verifyPassword(password, stored_hash);
        }
    }
    sqlite3_finalize(stmt);

    if (ok)
    {
        logAction("Verified user " + login);
    }
    else
    {
        logAction("Failed to verify user " + login);
    }
    return ok;
}

std::vector<std::string> ChatDB::getUserMessages(const std::string &login)
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;
    std::vector<std::string> messages;

    const char *sql = "SELECT u.login, MAX(m.timestamp) \
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

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, login.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *message = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        const char *timestamp = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        if (message && timestamp)
        {
            messages.push_back(std::string(message) + " (" + std::string(timestamp) + ")");
        }
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<std::string> ChatDB::getUserList()
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;
    std::vector<std::string> loginUsers;
    const char *sql = "SELECT login FROM users";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Ошибка подготовки: " << sqlite3_errmsg(db) << std::endl;
        return {};
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *login = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (login)
        {
            loginUsers.push_back(login);
        }
    }

    sqlite3_finalize(stmt);
    return loginUsers;
}

void ChatDB::logAction(const std::string &action)
{
    std::lock_guard<std::recursive_mutex> lock(ChatDBMutex);
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO logs (action, timestamp) VALUES (?, datetime('now'));";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return;
    }

    sqlite3_bind_text(stmt, 1, action.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
