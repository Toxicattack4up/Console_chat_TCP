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

class Server {
private:
    int sock;
    struct sockaddr_in Addr;
    std::mutex clientsMutex;
    std::vector<int> clientSockets;
    std::map<int, std::string> clientLogins;
    std::unordered_map<std::string, std::pair<std::string, std::string>> credentials;
    std::vector<std::string> allMessages;
    std::map<std::string, std::vector<std::string>> privateMessages;
    int maxconnect = 10;
    static const size_t BUFFER_SIZE = 2048;

public:
    Server();
    ~Server();
    void run();
    void handleClient(int clientSocket);
    void closeClients(int clientSocket);
    void broadcastMessage(const std::string& message, int senderSocket);
    void privateMessage(const std::string& sender, const std::string& receiver, const std::string& content, int senderSocket);
    void addUser(const std::string& login, const std::string& password, const std::string& name);
    std::string findUser(const std::string& login);
    std::vector<std::string> getUserList();
private:
    std::string hashPassword(const std::string& password);
};