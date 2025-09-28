#include <sqlite3.h>
#include <string>
#include <mutex>
#include <vector>
#include <iostream>
#include <functional>
#include <sstream>

class DB {
public:
    DB(const std::string& db_name);
    DB() = delete;
    DB(const DB&) = delete;
    DB& operator=(const DB&) = delete;
    ~DB();

    
    void addMessage(const std::string& sender, const std::string& receiver, const std::string& content); //фиксация сообщения в БД
    std::vector<std::string> getMessages(const std::string& user1, const std::string& user2); //получение списка сообщений общего чата
    std::vector<std::string> getPublicMessages(); //сообщения общего чата

    int getUserId(const std::string& login);
    //добавление пользователя в БД
    void addUser(const std::string& login, const std::string& name, const std::string& password);
    //проверка пользователя в БД
    bool verifyUser(const std::string& login, const std::string& password);
    //получение истории сообщений
    std::vector<std::string> getUserMessages(const std::string& login);
    //получение списка пользователей
    std::vector<std::string> getUserList();

    //Логирование действий
    void logAction(const std::string& action);

private:
    sqlite3* db = nullptr;
    std::mutex dbMutex;
};