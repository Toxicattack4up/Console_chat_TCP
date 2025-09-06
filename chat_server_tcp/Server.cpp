#include "Server.h"
#include <filesystem>

namespace fs = std::filesystem;

Server::Server() 
{
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        std::cerr << "Error to create socket" << std::endl;
        exit(1);
    }

    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = INADDR_ANY;
    Addr.sin_port = htons(12345);

    if (bind(sock, (struct sockaddr*)&Addr, sizeof(Addr)) < 0) {
        std::cerr << "Error: fail to bind socket to port" << std::endl;
        exit(1);
    }

    if (listen(sock, maxconnect) < 0) {
        std::cerr << "Error: Failed to start listening" << std::endl;
        exit(1);
    }
    std::cout << "Server starts to 12345 port" << std::endl;
}

Server::~Server() 
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    close(sock);
    for (int s : clientSockets) close(s);
    std::cout << "Server shutdown" << std::endl;
}

void Server::run() 
{
    std::cout << "Server is running on port 12345" << std::endl;
    
    while (true) 
    {
        struct sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientSize);
        
        if (clientSocket < 0) 
        {
            std::cerr << "Failed to accept client connect" << std::endl;
            continue;
        }
        
        std::cout << "Client connected: " << clientSocket << std::endl;
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
        
        if (rec <= 0) 
        {
            std::cerr << "Client " << (authorized ? login : std::to_string(clientSocket)) << " disconnected." << std::endl;
            closeClients(clientSocket);
            return;
        }

        buffer[rec] = '\0';
        std::string message(buffer);

        if (!authorized) 
        {
            if (message.substr(0, 4) == "AUTH") 
            {
                std::istringstream iss(message.substr(5));
                std::string login_input, password_input;
                iss >> login_input >> password_input;
                std::cout << "login: " << login_input << ", Password: " << password_input << std::endl;
                if (findUser(login_input) == password_input) 
                {
                    authorized = true;
                    login = login_input;
                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        clientLogins[clientSocket] = login;
                    }
                    send(clientSocket, "AUTH_SUCCESS", 12, 0);
                    std::cout << "Client authenticated: " << login_input << std::endl;
                } else {
                    send(clientSocket, "AUTH_FAILED", 11, 0);
                }
            } else {
                send(clientSocket, "I'm expecting AUTH", 18, 0);
            }
        } else {
            if (message.substr(0, 3) == "ALL") 
            {
                std::string content = "[" + login + "] " + message.substr(4);
                broadcastMessage(content, clientSocket);
            } else if (message.substr(0, 7) == "PRIVATE") {
                std::istringstream iss(message.substr(8));
                std::string sender, receiver, content;
                iss >> sender >> receiver;
                std::getline(iss, content);
                content = content.empty() ? "" : content.substr(1);
                privateMessage(sender, receiver, content, clientSocket);
            } else {
                send(clientSocket, "Please authenticate first", 24, 0);
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
        close(clientSocket);
        std::cout << "Client disconnected" << std::endl;
    }
}

void Server::broadcastMessage(const std::string& message, int senderSocket) 
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (int socket : clientSockets) {
        if (socket != senderSocket) {
            send(socket, message.c_str(), message.size(), 0);
        }
    }

    std::cout << "Broadcasted message: " << message << std::endl;
}

void Server::privateMessage(const std::string& sender, const std::string& receiver, 
                   const std::string& content, int senderSocket) 
{
    std::string message = "[" + sender + " ->" + receiver + "]: " + content;
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (const auto& pair : clientLogins) {
        if (pair.second == receiver) {
            send(pair.first, message.c_str(), message.size(), 0);
            send(senderSocket, message.c_str(), message.size(), 0);
            std::cout << "Private message sent from " << sender << " to " << receiver << std::endl;
            return;
        }
    }
    send(senderSocket, "User not found", 14, 0);
}

void Server::addUser(const std::string& login, const std::string& password, const std::string& name) {
    std::string hashedPassword = hashPassword(password);
    credentials[login] = {hashedPassword, name};
    std::cout << "User added: " << login << std::endl;

}

std::string Server::findUser(const std::string& login) {
    auto it = credentials.find(login);
    if (it != credentials.end()) {
        return it->second.first;
    }
    return "";
}

std::vector<std::string> Server::getUserList() {
    std::vector<std::string> users;
    for (const auto& pair : credentials) {
        users.push_back(pair.first);
    }
    return users;
}

std::string Server::hashPassword(const std::string& password) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}