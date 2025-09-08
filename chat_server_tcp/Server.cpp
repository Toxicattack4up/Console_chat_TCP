#include "Server.h"


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
    std::cout << "Server is running on port 12345" << std::endl;
    
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
        
        if (rec <= 0) {
            std::cerr << "Client " << (authorized ? login : std::to_string(clientSocket)) << " disconnected." << std::endl;
            closeClients(clientSocket);
            return;
        }

        buffer[rec] = '\0';
        std::string message(buffer);

        if(message.size() > BUFFER_SIZE - 50) {
            send(clientSocket, "Message is too long", 16, 0);
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
                    if (send(clientSocket, "REGISTER_SUCCESS", 16, 0) < 0) {
                        std::cerr << "Error sending REGISTER_SUCCESS to client" << std::endl;
                        closeClients(clientSocket);
                        return;
                    }
                    std::cout << "Client registered: " << login_input << std::endl;

                } else {
                    if (send(clientSocket, "REGISTER_FAILED", 15, 0) < 0) {
                        std::cerr << "Error sending REGISTER_FAILED to client" << std::endl;
                        closeClients(clientSocket);
                        return;
                    }
                }
            } else if (message.substr(0, 4) == "AUTH")  {
                std::istringstream iss(message.substr(5));
                std::string login_input, password_input;
                iss >> login_input >> password_input;
                std::cout << "login: " << login_input << ", Password: " << password_input << std::endl;
                
                if (findUser(login_input) == hashPassword(password_input)) {
                    authorized = true;
                    login = login_input;
                    
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    clientLogins[clientSocket] = login;
                    
                    if (send(clientSocket, "AUTH_SUCCESS", 12, 0) < 0) {
                        std::cerr << "Error sending AUTH_SUCCESS to client" << std::endl;
                        closeClients(clientSocket);
                        return;
                    }
                    
                    std::cout << "Client authenticated: " << login_input << std::endl;
                    
                    for (const auto& msg : allMessages) {
                    if (send(clientSocket, msg.c_str(), msg.size(), 0) < 0) {
                        std::cerr << "Failed to send history to " << login << std::endl;
                        closeClients(clientSocket);
                        return;
                        }
                    }
                    for (const auto& msg : privateMessages[login]) {
                        if (send(clientSocket, msg.c_str(), msg.size(), 0) < 0) {
                            std::cerr << "Failed to send private history to " << login << std::endl;
                            closeClients(clientSocket);
                            return;
                        }
                    }
                } else {
                    if (send(clientSocket, "AUTH_FAILED", 11, 0) < 0) {
                        std::cerr << "Error sending AUTH_FAILED to client" << std::endl;
                        closeClients(clientSocket);
                        return;
                    }
                }
            } else {

                if (send(clientSocket, "I'm expecting AUTH", 18, 0) < 0) {
                    std::cerr << "Error sending AUTH request to client" << std::endl;
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
                if (send(clientSocket, userList.c_str(), userList.size(), 0) < 0) {
                    std::cerr << "Error sending user list to client" << std::endl;
                    closeClients(clientSocket);
                    return;
                }
        
            } else if (message == "EXIT") {

                std::cout << "Client " << login << " requested to exit." << std::endl;
                closeClients(clientSocket);
                return;
            } else {
                if (send(clientSocket, "Unknown command", 15, 0) < 0) {
                    std::cerr << "Error sending Unknown command to client" << std::endl;
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
        std::cout << "Client disconnected" << std::endl;
    }
}

void Server::broadcastMessage(const std::string& message, int senderSocket) 
{
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S] ") << message;
    std::string timestampedMessage = ss.str();
    allMessages.push_back(timestampedMessage);

    std::lock_guard<std::mutex> lock(clientsMutex);

    for (int socket : clientSockets) {
        if (socket != senderSocket) {
            if(send(socket, timestampedMessage.c_str(), timestampedMessage.size(), 0) < 0) {
                std::cerr << "Error sending message to client " << socket << std::endl;
                closeClients(socket);
            }
        }
    }

    std::cout << "Broadcasted message: " << timestampedMessage << std::endl;
}

void Server::privateMessage(const std::string& sender, const std::string& receiver, 
                   const std::string& content, int senderSocket) 
{
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S] ") << "[" << sender << " ->" << receiver << "]: " << content;
    std::string message = ss.str();
    privateMessages[receiver].push_back(message);
    privateMessages[sender].push_back(message);
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (const auto& pair : clientLogins) {
        if (pair.second == receiver) {
            if(send(pair.first, message.c_str(), message.size(), 0) < 0) {
                std::cerr << "Error sending private message to " << receiver << std::endl;
                closeClients(pair.first);
            }
            if (send(senderSocket, message.c_str(), message.size(), 0) < 0) {
                std::cerr << "Error sending private message to sender" << std::endl;
                closeClients(senderSocket);
            }
            std::cout << "Private message sent from " << sender << " to " << receiver << std::endl;
            return;
        }
    }
    send(senderSocket, "User not found", 14, 0);
}

void Server::addUser(const std::string& login, const std::string& password, const std::string& name) {
    if(findUser(login) != "") {
        std::cerr << "User already exists: " << login << std::endl;
        return;
    }
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