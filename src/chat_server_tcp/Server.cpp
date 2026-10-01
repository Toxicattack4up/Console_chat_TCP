#include "Server.h"
#include <algorithm>
#include <vector>

bool Server::send_line(int sock, const std::string &msg)
{
    std::string out = msg + "\n";
    const char *data = out.c_str();
    size_t left = out.size();
    while (left > 0)
    {
#ifdef _WIN32
        const int sent = ::send(sock, data, static_cast<int>(left), 0);
#else
        const ssize_t sent = ::send(sock, data, left, 0);
#endif
        if (sent < 0)
        {
            return false;
        }
        data += sent;
        left -= static_cast<size_t>(sent);
    }
    return true;
}

std::string Server::getCurrentTimestamp()
{
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S]");
    return ss.str();
}

Server::Server() : db("tcp_chat.db")
{
    if (db.getUserId("admin") == -1)
    {
        db.addUser("admin", "Administrator", "admin123");
    }

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "Winsock error";
        exit(1);
    }
#endif

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt)) < 0)
    {
        std::cerr << "Warning: setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
    }

    Addr.sin_family = AF_INET;
    Addr.sin_addr.s_addr = INADDR_ANY;
    Addr.sin_port = htons(12345);

    if (bind(sock, (struct sockaddr *)&Addr, sizeof(Addr)) < 0)
    {
        std::cerr << "Error: fail to bind socket to port: " << strerror(errno) << std::endl;
        exit(1);
    }

    if (listen(sock, maxconnect) < 0)
    {
        std::cerr << "Error: Failed to start listening" << std::endl;
        exit(1);
    }

    logInfo("Server listening on port 12345");
    db.logAction("Server listening on port 12345");
}

Server::~Server()
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    db.logAction("Server shutting down");

#ifdef _WIN32
    closesocket(sock);
    for (int s : clientSockets)
        closesocket(s);
    WSACleanup();
#else
    close(sock);
    for (int s : clientSockets)
        close(s);
#endif
}

void Server::run()
{
    while (true)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(sock, (struct sockaddr *)&clientAddr, &clientSize);

        if (clientSocket < 0)
        {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        db.logAction(std::string("Client connected socket=") + std::to_string(clientSocket) + " ip=" + clientIP);

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
    std::string lineBuffer;

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        const int rec = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (rec <= 0)
        {
            handleClientDisconnect(clientSocket, authorized, login);
            return;
        }

        lineBuffer.append(buffer, static_cast<size_t>(rec));

        std::size_t pos;
        while ((pos = lineBuffer.find('\n')) != std::string::npos)
        {
            std::string message = lineBuffer.substr(0, pos);
            lineBuffer.erase(0, pos + 1);
            if (!message.empty() && message.back() == '\r')
            {
                message.pop_back();
            }

            if (message.size() > BUFFER_SIZE - 50)
            {
                send_line(clientSocket, "Message is too long");
                continue;
            }

            if (!authorized)
            {
                handleUnauthorizedClient(clientSocket, message, authorized, login);
            }
            else
            {
                handleAuthorizedClient(clientSocket, login, message);
                if (message == "EXIT")
                {
                    authorized = false;
                    login.clear();
                }
            }
        }
    }
}

void Server::handleClientDisconnect(int clientSocket, bool authorized, const std::string &login)
{
    std::string who = authorized ? login : std::to_string(clientSocket);
    db.logAction("Client disconnected: " + who);
    closeClients(clientSocket);
}

void Server::handleUnauthorizedClient(int clientSocket, const std::string &message, bool &authorized, std::string &login)
{
    if (message.rfind("REGISTER", 0) == 0)
    {
        handleRegistration(clientSocket, message);
    }
    else if (message.rfind("AUTH", 0) == 0)
    {
        handleAuthorization(clientSocket, message, authorized, login);
    }
    else
    {
        if (!send_line(clientSocket, "I'm expecting AUTH or REGISTER"))
        {
            closeClients(clientSocket);
        }
    }
}

void Server::handleAuthorizedClient(int clientSocket, const std::string &login,
                                    const std::string &message)
{
    if (message.rfind("ALL", 0) == 0)
    {
        handlePublicMessage(clientSocket, login, message);
    }
    else if (message.rfind("PRIVATE", 0) == 0)
    {
        handlePrivateMessage(clientSocket, login, message);
    }
    else if (message == "GET_USERS")
    {
        handleGetUsers(clientSocket, login);
    }
    else if (message == "EXIT")
    {
        handleClientExit(clientSocket, login);
    }
    else if (message == "GET_HISTORY")
    {
        handleGetHistory(clientSocket, login);
    }
    else if (message.rfind("GET_PRIVATE", 0) == 0)
    {
        handleGetPrivateHistory(clientSocket, login, message);
    }
    else
    {
        handleUnknownCommand(clientSocket, login);
    }
}

void Server::handleRegistration(int clientSocket, const std::string &message)
{
    // Protocol: REGISTER <login> <username> <password>
    std::istringstream iss(message.substr(8));
    std::string login, name, password;
    iss >> login >> name >> password;

    if (login.empty() || name.empty() || password.empty())
    {
        send_line(clientSocket, "REGISTER_FAILED: All fields required");
        return;
    }

    if (login.length() < 3 || password.length() < 3)
    {
        send_line(clientSocket, "REGISTER_FAILED: Login and password must be at least 3 characters");
        return;
    }

    if (db.getUserId(login) != -1)
    {
        send_line(clientSocket, "REGISTER_FAILED: Login already exists");
        return;
    }

    if (!db.addUser(login, name, password))
    {
        send_line(clientSocket, "REGISTER_FAILED: Could not create user");
        return;
    }
    send_line(clientSocket, "REGISTER_SUCCESS");
    db.logAction("Registered user: " + login);
}

void Server::handleAuthorization(int clientSocket, const std::string &message, bool &authorized, std::string &login)
{
    std::istringstream iss(message.substr(4));
    std::string login_input, password_input;
    iss >> login_input >> password_input;

    db.logAction("Auth attempt: " + login_input);

    if (db.verifyUser(login_input, password_input))
    {
        authorized = true;
        login = login_input;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientLogins[clientSocket] = login;
        }
        send_line(clientSocket, "AUTH_SUCCESS");
        db.logAction("Auth success: " + login_input);
    }
    else
    {
        send_line(clientSocket, "AUTH_FAILED: Invalid credentials");
        db.logAction("Auth failed: " + login_input);
    }
}

void Server::handlePublicMessage(int clientSocket, const std::string &login, const std::string &message)
{
    std::string content = message.size() > 3 ? message.substr(3) : "";
    if (!content.empty() && content[0] == ' ')
    {
        content.erase(0, 1);
    }

    db.addMessage(login, "", content);

    std::string formattedContent = getCurrentTimestamp() + " [" + login + "] " + content;
    broadcastMessage(formattedContent, clientSocket);
}

void Server::handlePrivateMessage(int clientSocket, const std::string &login,
                                  const std::string &message)
{
    std::istringstream iss(message.substr(7));
    std::string receiver, content;
    iss >> receiver;
    std::getline(iss, content);
    if (!content.empty() && content[0] == ' ')
    {
        content.erase(0, 1);
    }

    if (receiver.empty())
    {
        send_line(clientSocket, "PRIVATE_FAILED: Receiver required");
        return;
    }

    if (db.getUserId(receiver) == -1)
    {
        send_line(clientSocket, "PRIVATE_FAILED: User not found");
        return;
    }

    db.addMessage(login, receiver, content);
    privateMessage(login, receiver, content, clientSocket);
}

void Server::handleGetUsers(int clientSocket, const std::string &login)
{
    auto users = db.getUserList();
    std::string userList = "USERS";
    for (const auto &user : users)
    {
        userList += " " + user;
    }

    if (!send_line(clientSocket, userList))
    {
        db.logAction("Failed to send user list to " + login);
        closeClients(clientSocket);
    }
}

void Server::handleClientExit(int clientSocket, const std::string &login)
{
    db.logAction("Client exit: " + login);
    std::lock_guard<std::mutex> lock(clientsMutex);
    clientLogins.erase(clientSocket);
}

void Server::handleGetHistory(int clientSocket, const std::string &login)
{
    auto history = db.getPublicMessages();

    for (const auto &msg : history)
    {
        if (!send_line(clientSocket, msg))
        {
            closeClients(clientSocket);
            return;
        }
    }

    send_line(clientSocket, "END_OF_HISTORY");
}

void Server::handleGetPrivateHistory(int clientSocket, const std::string &login,
                                     const std::string &message)
{
    std::istringstream iss(message.substr(12));
    std::string other;
    iss >> other;

    if (other.empty())
    {
        send_line(clientSocket, "PRIVATE_HISTORY_FAILED: User required");
        return;
    }

    auto privateHistory = db.getMessages(login, other);

    for (const auto &msg : privateHistory)
    {
        if (!send_line(clientSocket, msg))
        {
            closeClients(clientSocket);
            return;
        }
    }

    send_line(clientSocket, "END_OF_HISTORY");
}

void Server::handleUnknownCommand(int clientSocket, const std::string &login)
{
    if (!send_line(clientSocket, "Unknown command"))
    {
        closeClients(clientSocket);
    }
}

void Server::closeClients(int clientSocket)
{
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
        if (it != clientSockets.end())
        {
            clientSockets.erase(it);
        }
        clientLogins.erase(clientSocket);
    }
    socketClose(clientSocket);
}

void Server::socketClose(int clientSocket)
{
#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

void Server::broadcastMessage(const std::string &message, int senderSocket)
{
    std::vector<int> targets;
    std::vector<int> failed;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (int socket : clientSockets)
        {
            if (socket != senderSocket)
            {
                targets.push_back(socket);
            }
        }
    }

    for (int socket : targets)
    {
        if (!send_line(socket, message))
        {
            failed.push_back(socket);
        }
    }

    for (int socket : failed)
    {
        closeClients(socket);
    }
}

void Server::privateMessage(const std::string &sender, const std::string &receiver,
                            const std::string &content, int senderSocket)
{
    std::string message = getCurrentTimestamp() + " [" + sender + " -> " + receiver + "]: " + content;

    int receiverSocket = -1;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto &pair : clientLogins)
        {
            if (pair.second == receiver)
            {
                receiverSocket = pair.first;
                break;
            }
        }
    }

    std::vector<int> failed;
    bool delivered = false;

    if (receiverSocket >= 0)
    {
        if (send_line(receiverSocket, message))
        {
            delivered = true;
        }
        else
        {
            failed.push_back(receiverSocket);
        }
    }

    if (receiver != sender)
    {
        if (!send_line(senderSocket, message))
        {
            failed.push_back(senderSocket);
        }
    }

    for (int socket : failed)
    {
        closeClients(socket);
    }

    if (!delivered)
    {
        send_line(senderSocket, "User is offline");
    }
}
