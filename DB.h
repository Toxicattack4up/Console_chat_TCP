#include <sqlite3.h>
#include <string>
#include <mutex>
#include <vector>
#include <iostream>

class DB {
public:
    DB(const std::string& db_name);
    ~DB();

    //фиксация сообщения в БД
    void addMessage(const std::string& sender, const std::string& receiver, const std::string& content);
    //получение списка сообщений
    std::vector<std::string> getMessages(const std::string& user1, const std::string& user2);

    int getUserId(const std::string& login);
    //добавление пользователя в БД
    void addUser(const std::string& login, const std::string& password, const std::string& name);
    //проверка пользователя в БД
    bool verifyUser(const std::string& login, const std::string& password);
    //получение истории сообщений
    std::vector<std::string> getUserMessages(const std::string& login);
    //получение списка пользователей
    std::vector<std::string> getUserList();


    //Логирование действий
    void logAction(const std::string& action);

private:
    sqlite3* db;
    std::mutex dbMutex;

    
};