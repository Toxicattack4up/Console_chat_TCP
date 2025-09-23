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

class Client {
private:
    int clientSock;
    int serverPort = 12345;
    bool isThreadRunning = false;
    bool running = true;
    static const size_t BUFFER_SIZE = 2048;
    bool waitingForHistory = false;
    std::vector<std::string> historyLines;
    std::mutex history_mutex;
    std::condition_variable history_cv;
        // private history response
        bool waitingForPrivateHistory = false;
        std::vector<std::string> privateHistoryLines;
        std::mutex private_history_mutex;
        std::condition_variable private_history_cv;
    // GET_USERS synchronization
    bool waitingForUsers = false;
    std::string usersResponse;
    std::mutex users_mutex;
    std::condition_variable users_cv;
    // Recent messages buffer to avoid printing duplicates when showing history
    std::deque<std::string> recentMessages;
    std::mutex recent_mutex;
    size_t recentLimit = 100;
    // buffer for partial incoming data
    std::string incomingBuffer;
    
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
    void requestHistory();
    void requestPrivateHistory(const std::string &other);
    void receiveMessages();
    void startThread();
    void stopReceivedMessage();
    void disconnect();
};