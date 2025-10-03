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
    Server();
    ~Server();
    void run();
    void handleClient(int clientSocket);
    void closeClients(int clientSocket);
    void broadcastMessage(const std::string &message, int senderSocket);
    void privateMessage(const std::string &sender, const std::string &receiver, const std::string &content, int senderSocket);

private:
    void handleClientDisconnect(int clientSocket, bool authorized, const std::string &login);
    void handleUnauthorizedClient(int clientSocket, const std::string &message, bool &authorized, std::string &login);
    void handleAuthorizedClient(int clientSocket, const std::string &login, const std::string &message);
    void handleRegistration(int clientSocket, const std::string &message);
    void handleAuthorization(int clientSocket, const std::string &message, bool &authorized, std::string &login);
    void handlePublicMessage(int clientSocket, const std::string &login, const std::string &message);
    void handlePrivateMessage(int clientSocket, const std::string &login, const std::string &message);
    void handleGetUsers(int clientSocket, const std::string &login);
    void handleClientExit(int clientSocket, const std::string &login);
    void handleGetHistory(int clientSocket, const std::string &login);
    void handleGetPrivateHistory(int clientSocket, const std::string &login, const std::string &message);
    void handleUnknownCommand(int clientSocket, const std::string &login);

    static bool send_line(int sock, const std::string &msg);
    std::string getCurrentTimestamp();
};