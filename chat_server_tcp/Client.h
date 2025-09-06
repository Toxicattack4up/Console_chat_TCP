#pragma once
#include <iostream>
#include <cstring>
#include <string>
#include <thread>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/types.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Client {
private:
    int clientSock;
    int serverPort = 12345;
    bool isThreadRunning = false;
    bool running = true;
    static const size_t BUFFER_SIZE = 2048;
    
public:
    Client();
    ~Client();
    bool sendRegister(const std::string& login, const std::string& password, const std::string& name);
    void connectToServer(const std::string& ip_to_server);
    void isConnected();
    std::vector<std::string> getListOfUsers();
    bool sendAUTH(const std::string& login, const std::string& password);
    bool sendToAll(const std::string& message);
    void sendPrivate(const std::string& sender, const std::string& receiver, const std::string& message);
    void receiveMessages();
    void startThread();
    void stopReceivedMessage();
    void disconnect();
};