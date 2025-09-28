#include "Server.h"

bool Server::send_line(int sock, const std::string &msg) {
    std::string out = msg + "\n";
    ssize_t r = send(sock, out.c_str(), out.size(), 0);
    return r >= 0;
}

std::string Server::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S]");
    return ss.str();
}

Server::Server() : db("tcp_chat.db") {
    // Создаем администратора при первом запуске
    if (db.getUserId("admin") == -1) {
        db.addUser("admin", "Администратор", "admin123");
    }
    
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { 
            std::cerr << "Winsock error"; 
            exit(1); 
        }
    #endif

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(1);
    }

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
    db.logAction("Сервер запущен на порту 12345");
}

Server::~Server() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    db.logAction("Сервер завершается, отключаем клиентов");
    
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

void Server::run() {
    logInfo("Server is running on port 12345");
    db.logAction("Сервер запущен и ожидает подключения клиентов");
    
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr*)&clientAddr, &clientSize);
        
        if (clientSocket < 0) {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }
        
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        db.logAction(std::string("Клиент подключен: ") + std::to_string(clientSocket) + " с IP " + clientIP);

        std::lock_guard<std::mutex> lock(clientsMutex);
        clientSockets.push_back(clientSocket);
        std::thread(&Server::handleClient, this, clientSocket).detach();
    }
}

void Server::handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    bool authorized = false;
    std::string login;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int rec = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (rec <= 0) {
            handleClientDisconnect(clientSocket, authorized, login);
            return;
        }

        buffer[rec] = '\0';
        std::string message(buffer);

        if (message.size() > BUFFER_SIZE - 50) {
            send_line(clientSocket, "Message is too long");
            continue;
        }

        if (!authorized) {
            handleUnauthorizedClient(clientSocket, message, authorized, login);
        } else {
            handleAuthorizedClient(clientSocket, login, message);
        }
    }    
}

void Server::handleClientDisconnect(int clientSocket, bool authorized, const std::string& login) {
    std::string logMsg = "Client disconnected: " + (authorized ? login : std::to_string(clientSocket));
    logInfo(logMsg);
    db.logAction("Клиент отключен: " + (authorized ? login : std::to_string(clientSocket)));
    closeClients(clientSocket);
}

void Server::handleUnauthorizedClient(int clientSocket, const std::string& message, 
                                     bool& authorized, std::string& login) {
    if (message.substr(0, 8) == "REGISTER") {
        handleRegistration(clientSocket, message);
    } else if (message.substr(0, 4) == "AUTH") {
        handleAuthorization(clientSocket, message, authorized, login);
    } else {
        if (!send_line(clientSocket, "I'm expecting AUTH or REGISTER")) {
            logError("Error sending auth request to client");
            db.logAction("Ошибка отправки запроса авторизации клиенту " + std::to_string(clientSocket));
            closeClients(clientSocket);
        }
    }
}

void Server::handleAuthorizedClient(int clientSocket, const std::string& login, 
                                   const std::string& message) {
    if (message.substr(0, 3) == "ALL") {
        handlePublicMessage(clientSocket, login, message);
    } else if (message.substr(0, 7) == "PRIVATE") {
        handlePrivateMessage(clientSocket, login, message);
    } else if (message == "GET_USERS") {
        handleGetUsers(clientSocket, login);
    } else if (message == "EXIT") {
        handleClientExit(clientSocket, login);
    } else if (message == "GET_HISTORY") {
        handleGetHistory(clientSocket, login);
    } else if (message.rfind("GET_PRIVATE", 0) == 0) {
        handleGetPrivateHistory(clientSocket, login, message);
    } else {
        handleUnknownCommand(clientSocket, login);
    }
}

void Server::handleRegistration(int clientSocket, const std::string& message) {
    std::istringstream iss(message.substr(9));
    std::string login, name, password;
    iss >> login >> name >> password;
    std::getline(iss, name);
    
    if (login.empty() || name.empty() || password.empty()) {
        send_line(clientSocket, "REGISTER_FAILED: All fields required");
        return;
    }

    if (login.length() < 3 || password.length() < 3) {
        send_line(clientSocket, "REGISTER_FAILED: Login and password must be at least 3 characters");
        return;
    }

    if (db.getUserId(login) != -1) {
        send_line(clientSocket, "REGISTER_FAILED: Login already exists");
        return;
    }

    db.addUser(login, name, password);
    send_line(clientSocket, "REGISTER_SUCCESS");
    db.logAction("Зарегистрирован новый пользователь: " + login);
}

void Server::handleAuthorization(int clientSocket, const std::string& message, 
                                bool& authorized, std::string& login) {
    std::istringstream iss(message.substr(5));
    std::string login_input, password_input;
    iss >> login_input >> password_input;

    db.logAction("Попытка авторизации клиента: " + login_input);
    
    if (db.verifyUser(login_input, password_input)) {
        authorized = true;
        login = login_input;

        std::lock_guard<std::mutex> lock(clientsMutex);
        clientLogins[clientSocket] = login;

        if (send_line(clientSocket, "AUTH_SUCCESS")) {
            logInfo("Client authenticated: " + login_input);
            db.logAction("Клиент авторизован: " + login_input);
        } else {
            logError("Error sending AUTH_SUCCESS to client");
            closeClients(clientSocket);
        }
    } else {
        send_line(clientSocket, "AUTH_FAILED: Invalid credentials");
        db.logAction("Неудачная авторизация: " + login_input);
    }
}

void Server::handlePublicMessage(int clientSocket, const std::string& login, 
                                const std::string& message) {
    std::string content = message.substr(4);
    
    // Сохраняем в базу (receiver = "" для общего чата)
    db.addMessage(login, "", content);
    
    // Рассылаем всем
    std::string formattedContent = getCurrentTimestamp() + " [" + login + "] " + content;
    broadcastMessage(formattedContent, clientSocket);
}

void Server::handlePrivateMessage(int clientSocket, const std::string& login, 
                                 const std::string& message) {
    std::istringstream iss(message.substr(8));
    std::string receiver, content;
    iss >> receiver;
    std::getline(iss, content);
    content = content.empty() ? "" : content.substr(1);

    if (receiver.empty()) {
        send_line(clientSocket, "PRIVATE_FAILED: Receiver required");
        return;
    }

    // Проверяем существование получателя
    if (db.getUserId(receiver) == -1) {
        send_line(clientSocket, "PRIVATE_FAILED: User not found");
        return;
    }

    // Сохраняем в базу
    db.addMessage(login, receiver, content);
    
    // Отправляем приватное сообщение
    privateMessage(login, receiver, content, clientSocket);
}

void Server::handleGetUsers(int clientSocket, const std::string& login) {
    auto users = db.getUserList();
    std::string userList = "USERS";
    for (const auto& user : users) {
        userList += " " + user;
    }
    
    if (!send_line(clientSocket, userList)) {
        logError("Error sending user list to client");
        db.logAction("Ошибка отправки списка пользователей клиенту " + login);
        closeClients(clientSocket);
    }
}

void Server::handleClientExit(int clientSocket, const std::string& login) {
    logInfo("Client requested exit: " + login);
    db.logAction("Клиент запросил выход: " + login);
    closeClients(clientSocket);
}

void Server::handleGetHistory(int clientSocket, const std::string& login) {
    // Получаем историю общего чата из базы
    auto history = db.getPublicMessages();
    
    for (const auto& msg : history) {
        if (!send_line(clientSocket, msg)) {
            logError("Failed to send history to " + login);
            db.logAction("Ошибка отправки истории клиенту " + login);
            closeClients(clientSocket);
            return;
        }
    }
    
    send_line(clientSocket, "END_OF_HISTORY");
}

void Server::handleGetPrivateHistory(int clientSocket, const std::string& login, 
                                    const std::string& message) {
    std::istringstream iss(message.substr(12));
    std::string other;
    iss >> other;

    if (other.empty()) {
        send_line(clientSocket, "PRIVATE_HISTORY_FAILED: User required");
        return;
    }

    // Получаем приватную историю из базы
    auto privateHistory = db.getMessages(login, other);
    
    for (const auto& msg : privateHistory) {
        if (!send_line(clientSocket, msg)) {
            logError("Failed to send private history to " + login);
            db.logAction("Ошибка отправки приватной истории клиенту " + login);
            closeClients(clientSocket);
            return;
        }
    }
    
    send_line(clientSocket, "END_OF_HISTORY");
}

void Server::handleUnknownCommand(int clientSocket, const std::string& login) {
    if (!send_line(clientSocket, "Unknown command")) {
        logError("Error sending unknown command response");
        db.logAction("Ошибка отправки ответа на неизвестную команду клиенту " + login);
        closeClients(clientSocket);
    }
}

void Server::closeClients(int clientSocket) {
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
        logInfo("Client socket closed: " + std::to_string(clientSocket));
    }
}

void Server::broadcastMessage(const std::string& message, int senderSocket) {
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (int socket : clientSockets) {
        if (socket != senderSocket) { // Не отправляем отправителю
            if (!send_line(socket, message)) {
                logError("Error sending message to client " + std::to_string(socket));
                closeClients(socket);
            }
        }
    }

    logInfo("Broadcasted message: " + message);
}

void Server::privateMessage(const std::string& sender, const std::string& receiver, 
                           const std::string& content, int senderSocket) {
    std::string message = getCurrentTimestamp() + " [" + sender + " -> " + receiver + "]: " + content;

    std::lock_guard<std::mutex> lock(clientsMutex);
    bool delivered = false;
    
    // Отправляем получателю
    for (const auto& pair : clientLogins) {
        if (pair.second == receiver) {
            if (send_line(pair.first, message)) {
                delivered = true;
            } else {
                logError("Error sending private message to " + receiver);
                closeClients(pair.first);
            }
            break;
        }
    }
    
    // Отправляем отправителю (если не самому себе)
    if (receiver != sender) {
        if (!send_line(senderSocket, message)) {
            logError("Error sending private message to sender");
            closeClients(senderSocket);
        }
    }
    
    if (delivered) {
        logInfo("Private message sent from " + sender + " to " + receiver);
    } else {
        send_line(senderSocket, "User is offline");
    }
}