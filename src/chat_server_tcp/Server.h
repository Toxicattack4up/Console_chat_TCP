#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <map>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <cerrno>
#include <cstring>
#include "../include/common.h"
#include "Database.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// Сервер TCP-чата: принимает подключения, авторизует и маршрутизирует сообщения
class Server
{
private:
    int sock;
    struct sockaddr_in Addr;
    std::mutex clientsMutex;
    std::vector<int> clientSockets;
    std::map<int, std::string> clientLogins;
    int maxconnect = 10;
    static const size_t BUFFER_SIZE = 2048;
    ChatDB db;

public:
    // Инициализация сокета и запуск прослушивания
    Server();
    // Корректное завершение, закрытие сокетов
    ~Server();
    // Главный цикл приёма подключений
    void run();
    // Обработка команд конкретного клиента
    void handleClient(int clientSocket);
    // Закрытие данных клиента и удаление из списков
    void closeClients(int clientSocket);
    // Низкоуровневое закрытие сокета
    void socketClose(int clientSocket);
    // Рассылка сообщения всем, кроме отправителя
    void broadcastMessage(const std::string &message, int senderSocket);
    // Отправка приватного сообщения получателю и отправителю
    void privateMessage(const std::string &sender, const std::string &receiver, const std::string &content, int senderSocket);


    
private:
    // Фиксация отключения клиента
    void handleClientDisconnect(int clientSocket, bool authorized, const std::string &login);
    // Обработка команд до авторизации
    void handleUnauthorizedClient(int clientSocket, const std::string &message, bool &authorized, std::string &login);
    // Обработка команд после авторизации
    void handleAuthorizedClient(int clientSocket, const std::string &login, const std::string &message);
    // Регистрация нового пользователя
    void handleRegistration(int clientSocket, const std::string &message);
    // Авторизация пользователя
    void handleAuthorization(int clientSocket, const std::string &message, bool &authorized, std::string &login);
    // Публичное сообщение всем
    void handlePublicMessage(int clientSocket, const std::string &login, const std::string &message);
    // Приватное сообщение одному пользователю
    void handlePrivateMessage(int clientSocket, const std::string &login, const std::string &message);
    // Отправка списка пользователей
    void handleGetUsers(int clientSocket, const std::string &login);
    // Обработка запроса выхода
    void handleClientExit(int clientSocket, const std::string &login);
    // История общего чата
    void handleGetHistory(int clientSocket, const std::string &login);
    // История приватной переписки
    void handleGetPrivateHistory(int clientSocket, const std::string &login, const std::string &message);
    // Ответ на неизвестную команду
    void handleUnknownCommand(int clientSocket, const std::string &login);
    // Отправка строки с переводом строки
    static bool send_line(int sock, const std::string &msg);
    // Текущая временная метка
    std::string getCurrentTimestamp();
};