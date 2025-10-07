#pragma once
#include <sqlite3.h>
#include <string>
#include <mutex>
#include <vector>
#include <iostream>
#include <functional>
#include <sstream>

class ChatDB
{
public:
    // Открывает/создаёт файл БД и готовит таблицы
    ChatDB(const std::string &ChatDB_name);
    ChatDB() = delete;
    ChatDB(const ChatDB &) = delete;
    ChatDB &operator=(const ChatDB &) = delete;
    ~ChatDB();

    // Сохраняет сообщение (receiver пустой — сообщение общего чата)
    void addMessage(const std::string &sender, const std::string &receiver, const std::string &content);
    // Возвращает переписку между двумя пользователями
    std::vector<std::string> getMessages(const std::string &user1, const std::string &user2);
    // Возвращает историю общего чата
    std::vector<std::string> getPublicMessages();

    // Возвращает id пользователя по логину (или -1)
    int getUserId(const std::string &login);
    // Добавляет пользователя
    void addUser(const std::string &login, const std::string &name, const std::string &password);
    // Проверяет логин/пароль
    bool verifyUser(const std::string &login, const std::string &password);
    // Возвращает список собеседников и время последнего сообщения
    std::vector<std::string> getUserMessages(const std::string &login);
    // Возвращает список логинов
    std::vector<std::string> getUserList();

    // Записывает событие в таблицу логов
    void logAction(const std::string &action);

private:
    sqlite3 *db = nullptr;
    std::mutex ChatDBMutex;
};