#pragma once
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <sstream>
#include "../include/common.h"
#include "../chat_server_tcp/Server.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Клиент TCP-чата: подключение/авторизация и обмен сообщениями
class Client
{
private:
    int clientSock;
    int serverPort = 12345;
    std::string serverIp;
    bool isThreadRunning = false;
    bool running = false;
    static const size_t BUFFER_SIZE = 2048;
    bool waitingForHistory = false;
    std::vector<std::string> historyLines;
    std::mutex history_mutex;
    std::condition_variable history_cv;

    bool waitingForPrivateHistory = false;
    std::vector<std::string> privateHistoryLines;
    std::mutex private_history_mutex;
    std::condition_variable private_history_cv;

    bool waitingForUsers = false;
    std::string usersResponse;
    std::mutex users_mutex;
    std::condition_variable users_cv;

    std::deque<std::string> recentMessages;
    std::mutex recent_mutex;
    size_t recentLimit = 100;

    std::string incomingBuffer;

public:
    // Инициализация клиента
    Client();
    // Завершение работы
    ~Client();
    // Регистрация пользователя
    bool sendRegister(const std::string &login, const std::string &username, const std::string &password);
    // Подключение к серверу по IPv4
    void connectToServer(const std::string &ip_to_server);
    // Гарантированное подключение (переподключение при необходимости)
    bool ensureConnected();
    // Печать статуса подключения
    void isConnected();
    // Получение списка пользователей
    std::vector<std::string> getListOfUsers();
    // Авторизация
    bool sendAUTH(const std::string &login, const std::string &password);
    // Сообщение в общий чат
    bool sendToAll(const std::string &message);
    // Приватное сообщение пользователю
    void sendPrivate(const std::string &sender, const std::string &receiver, const std::string &message);
    // История общего чата
    void requestHistory();
    // История приватной переписки
    void requestPrivateHistory(const std::string &other);
    // Цикл приёма сообщений (фон)
    void receiveMessages();
    // Запуск фонового приёма
    void startThread();
    // Остановка фонового приёма
    void stopReceivedMessage();
    // Разрыв соединения
    void disconnect();
    // Отправка выхода и остановка приёма
    void logOut();
};