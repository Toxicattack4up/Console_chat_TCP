#include "Server.h"
#include <cerrno>
#include <cstring>
#include "../common.h"
#include <../DB.h>

// send a single line (terminated with '\n') to make framing simple
static bool send_line(int sock, const std::string &msg) {
    std::string out = msg + "\n";
    ssize_t r = send(sock, out.c_str(), out.size(), 0);
    return r >= 0;
}


Server::Server() 
{
    credentials["admin"] = {hashPassword("admin123"), "Admin"};
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { std::cerr << "Winsock error"; exit(1); }
    #endif

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(1);
    }

    // allow quick reuse of the address/port (avoid bind failures on restart)
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Warning: setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
    }

    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = INADDR_ANY;
    Addr.sin_port = htons(12345);

    if (bind(sock, (struct sockaddr*)&Addr, sizeof(Addr)) < 0) {
        std::cerr << "Error: fail to bind socket to port: " << strerror(errno) << std::endl;
        exit(1);
    }

    if (listen(sock, maxconnect) < 0) {
        std::cerr << "Error: Failed to start listening" << std::endl;
        exit(1);
    }
    logInfo("Server starts to 12345 port");
}

Server::~Server() 
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    #ifdef _WIN32
        closesocket(sock);
        for (int s : clientSockets) closesocket(s);
        WSACleanup();
    #else
        close(sock);
        for (int s : clientSockets) close(s);
    #endif    
    std::cout << "Server shutdown" << std::endl;
}

void Server::run() 
{
    logInfo("Server is running on port 12345");
    
    while (true) 
    {
        struct sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientSize);
        
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    logInfo(std::string("Client connected: ") + std::to_string(clientSocket));
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientSockets.push_back(clientSocket);
        }
        std::thread(&Server::handleClient, this, clientSocket).detach();
    }

}

void Server::handleClient(int clientSocket) 
{
    char buffer[BUFFER_SIZE];
    bool authorized = false;
    std::string login;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int rec = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (rec <= 0) {
            logInfo(std::string("Client disconnected: ") + (authorized ? login : std::to_string(clientSocket)));
            closeClients(clientSocket);
            return;
        }

        buffer[rec] = '\0';
        std::string message(buffer);

        if(message.size() > BUFFER_SIZE - 50) {
            send_line(clientSocket, "Message is too long");
            continue;
        }

        if (!authorized) {
            if (message.substr(0, 8) == "REGISTER") {
                std::istringstream iss(message.substr(9));
                std::string login_input, password_input, name_input;
                iss >> login_input >> password_input;
                std::getline(iss, name_input);
                name_input = name_input.empty() ? "" : name_input.substr(1);

                if(findUser(login_input).empty()) {
                    addUser(login_input, password_input, name_input);
                    if (!send_line(clientSocket, "REGISTER_SUCCESS")) {
                        logError("Error sending REGISTER_SUCCESS to client");
                        closeClients(clientSocket);
                        return;
                    }
                    logInfo(std::string("Client registered: ") + login_input);

                } else {
                    if (!send_line(clientSocket, "REGISTER_FAILED")) {
                        logError("Error sending REGISTER_FAILED to client");
                        closeClients(clientSocket);
                        return;
                    }
                }
                } else if (message.substr(0, 4) == "AUTH")  {
                std::istringstream iss(message.substr(5));
                std::string login_input, password_input;
                iss >> login_input >> password_input;
                logDebug(std::string("login attempt: ") + login_input);
                
                if (findUser(login_input) == hashPassword(password_input)) {
                    authorized = true;
                    login = login_input;

                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        clientLogins[clientSocket] = login;
                    }

                    if (!send_line(clientSocket, "AUTH_SUCCESS")) {
                        logError("Error sending AUTH_SUCCESS to client");
                        closeClients(clientSocket);
                        return;
                    }

                    logInfo(std::string("Client authenticated: ") + login_input);
                } else {
                    if (!send_line(clientSocket, "AUTH_FAILED")) {
                        logError("Error sending AUTH_FAILED to client");
                        closeClients(clientSocket);
                        return;
                    }
                }
            } else {

                if (!send_line(clientSocket, "I'm expecting AUTH")) {
                    logError("Error sending AUTH request to client");
                    closeClients(clientSocket);
                    return;
                }
            }
        } else {
            if (message.substr(0, 3) == "ALL") {

                std::string content = "[" + login + "] " + message.substr(4);
                broadcastMessage(content, clientSocket);

            } else if (message.substr(0, 7) == "PRIVATE") {

                std::istringstream iss(message.substr(8));
                std::string receiver, content;
                iss >> receiver;
                std::getline(iss, content);
                content = content.empty() ? "" : content.substr(1);
                privateMessage(login, receiver, content, clientSocket);

            } else if (message == "GET_USERS") {

                auto users = getUserList();
                std::string userList = "USERS ";
                for (const auto& user : users) userList += user + " ";
                    if (!send_line(clientSocket, userList)) {
                    logError("Error sending user list to client");
                    closeClients(clientSocket);
                    return;
                }
        
            } else if (message == "EXIT") {

                logInfo(std::string("Client requested exit: ") + login);
                closeClients(clientSocket);
                return;
            } else if (message == "GET_HISTORY") {

                // send only public history
                std::vector<std::string> copyAll;
                {
                    std::lock_guard<std::mutex> dataLock(dataMutex);
                    copyAll = allMessages;
                }
                for (const auto& msg : copyAll) {
                    if (!send_line(clientSocket, msg)) {
                        logError(std::string("Failed to send history to ") + login);
                        closeClients(clientSocket);
                        return;
                    }
                }
                if (!send_line(clientSocket, std::string("END_OF_HISTORY"))) {
                    logError(std::string("Failed to send history terminator to ") + login);
                    closeClients(clientSocket);
                    return;
                }
            } else if (message.rfind("GET_PRIVATE", 0) == 0) {
                // format: GET_PRIVATE <otherUser>
                std::istringstream iss(message.substr(12));
                std::string other;
                iss >> other;
                std::vector<std::string> copyPrivate;
                {
                    std::lock_guard<std::mutex> dataLock(dataMutex);
                    // collect messages where sender or receiver equals login and other
                    auto itReq = privateMessages.find(login);
                    if (itReq != privateMessages.end()) copyPrivate = itReq->second;
                }
                // filter messages that mention the other user in the "->" pattern
                for (const auto& msg : copyPrivate) {
                    if (msg.find("->" + other) != std::string::npos || msg.find("->" + login) != std::string::npos) {
                        if (!send_line(clientSocket, msg)) {
                            logError(std::string("Failed to send private history to ") + login);
                            closeClients(clientSocket);
                            return;
                        }
                    }
                }
                if (!send_line(clientSocket, std::string("END_OF_HISTORY"))) {
                    logError(std::string("Failed to send history terminator to ") + login);
                    closeClients(clientSocket);
                    return;
                }
            } else {
                if (!send_line(clientSocket, "Unknown command")) {
                    logError("Error sending Unknown command to client");
                    closeClients(clientSocket);
                    return;
                }
            }
        }
    }    
}

void Server::closeClients(int clientSocket) 
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
        
    if (it != clientSockets.end()) {
        clientSockets.erase(it);
        clientLogins.erase(clientSocket);
        #ifdef _WIN32
            closesocket(clientSocket);
        #else
            close(clientSocket);
        #endif
        logInfo(std::string("Client socket closed: ") + std::to_string(clientSocket));
    }
}

void Server::broadcastMessage(const std::string& message, int senderSocket) 
{
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S] ") << message;
    std::string timestampedMessage = ss.str();
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        allMessages.push_back(timestampedMessage);
    }

    std::lock_guard<std::mutex> lock(clientsMutex);

    for (int socket : clientSockets) {
        if(!send_line(socket, timestampedMessage)) {
            logError(std::string("Error sending message to client ") + std::to_string(socket));
            closeClients(socket);
        }
    }

    logInfo(std::string("Broadcasted message: ") + timestampedMessage);
}

void Server::privateMessage(const std::string& sender, const std::string& receiver, 
                   const std::string& content, int senderSocket) 
{
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S] ") << "[" << sender << " ->" << receiver << "]: " << content;
    std::string message = ss.str();
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        // store message for both participants; avoid duplicate storage when sending to self
        privateMessages[receiver].push_back(message);
        if (receiver != sender) privateMessages[sender].push_back(message);
    }
    std::lock_guard<std::mutex> lock(clientsMutex);
    bool delivered = false;
    for (const auto& pair : clientLogins) {
        if (pair.second == receiver) {
            // send to receiver socket
            if(!send_line(pair.first, message)) {
                std::cerr << "Error sending private message to " << receiver << std::endl;
                closeClients(pair.first);
            }
            delivered = true;
            break;
        }
    }
    // send to sender as well if receiver != sender and senderSocket is still open
    if (receiver != sender) {
        if (!send_line(senderSocket, message)) {
            std::cerr << "Error sending private message to sender" << std::endl;
            closeClients(senderSocket);
        }
    }
    if (delivered) {
        logInfo(std::string("Private message sent from ") + sender + " to " + receiver);
        return;
    }
    send_line(senderSocket, "User not found");
}

void Server::addUser(const std::string& login, const std::string& password, const std::string& name) {
    if(findUser(login) != "") {
        std::cerr << "User already exists: " << login << std::endl;
        return;
    }
    std::string hashedPassword = hashPassword(password);
    {
        std::lock_guard<std::mutex> dataLock(dataMutex);
        credentials[login] = {hashedPassword, name};
    }
    std::cout << "User added: " << login << std::endl;

    logInfo(std::string("User added: ") + login);

}

std::string Server::findUser(const std::string& login) {
    std::lock_guard<std::mutex> dataLock(dataMutex);
    auto it = credentials.find(login);
    if (it != credentials.end()) {
        return it->second.first;
    }
    return "";
}

std::vector<std::string> Server::getUserList() {
    std::vector<std::string> users;
    std::lock_guard<std::mutex> dataLock(dataMutex);
    for (const auto& pair : credentials) {
        users.push_back(pair.first);
    }
    return users;
}

std::string Server::hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}