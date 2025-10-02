#include "Client.h"

Client::Client() {
    clientSock = -1;
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            exit(1);
        }
    #endif
    
}

Client::~Client() {
    disconnect();
    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "Client destroyed" << std::endl;
    }
}

bool Client::sendRegister(const std::string& login, const std::string& username, const std::string& password) {
    char buffer[BUFFER_SIZE];
    std::string registerMessageToServer = "REGISTER " + login + " " + username +  " " + password;
    std::string response;

    if(registerMessageToServer.size() > BUFFER_SIZE - 50) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "REGISTER message is too long" << std::endl;
        return false;
    }

    if(clientSock == -1 || !running) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Client is not connected" << std::endl;
        return false;
    } else {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "Client is connected" << std::endl;
    }

    if(send(clientSock, registerMessageToServer.c_str(), registerMessageToServer.length(), 0) < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error sending REGISTER message to server" << std::endl;
        return false;
    }

    while (true)
    {
        int bytesRead = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cerr << "Error receiving AUTH response" << std::endl;
            return false;
        }
        buffer[bytesRead] = '\0';
        response += buffer;
        if (response.find('\n') != std::string::npos) break;
    }
    
    response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());

    if (response == "REGISTER_SUCCESS") {
        logInfo("Регистрация прошла успешно для " + login);
        std::cout << "Регистрация прошла успешно для " << login << std::endl;
        startThread();
        return true;
    } else if (response == "REGISTER_FAILED") {
        logError("Ошибка регистрации для " + login);
        std::cout << "Ошибка регистрации для " << login << std::endl;
        return false;
    } else {
        logError("Неизвестный ответ от сервера: " + response);
        std::cout << "Неизвестный ответ от сервера: " << response << std::endl;
        return false;
    }

    return true;
}

void Client::connectToServer(const std::string& ip_to_server) {
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { std::cerr << "Winsock error"; exit(1); }
    #endif
    
    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSock < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error to create socket" << std::endl;
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    
    if(inet_pton(AF_INET, ip_to_server.c_str(), &serverAddress.sin_addr) <= 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Invalid IP address" << std::endl;
        exit(1);
    }
    serverAddress.sin_port = htons(serverPort);
    if (connect(clientSock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0) {
        logInfo("Connected to server");
    } else {
        logError("Error to connect server");
        exit(1);
    }
}

void Client::isConnected() {
    if(clientSock != -1 && running) {
        std::cout << "Client is connected" << std::endl;
    } else {
        std::cout << "Client is not connected" << std::endl;
    }
}

std::vector<std::string> Client::getListOfUsers() {
    char buffer[BUFFER_SIZE];
    std::string getUsersMessage = "GET_USERS";
    std::vector<std::string> users;
    // send request and wait for response in receiveMessages
    // prepare to wait for users response before sending to avoid race
    std::unique_lock<std::mutex> ulock(users_mutex);
    usersResponse.clear();
    waitingForUsers = true;

    if (send(clientSock, getUsersMessage.c_str(), getUsersMessage.length(), 0) < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error send GET_USERS message to server" << std::endl;
        return users;
    }
    // wait for users response
    
    std::unique_lock<std::mutex> lock(users_mutex);
    if (!users_cv.wait_for(ulock, std::chrono::seconds(5), [this]() { return !waitingForUsers; })) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Timeout waiting for USERS response" << std::endl;
        return users;
    }
    // parse usersResponse
    if (usersResponse.rfind("USERS ", 0) == 0) {
        std::string usersList = usersResponse.substr(6);
        std::istringstream iss(usersList);
        std::string user;
        while (iss >> user) users.push_back(user);
    } else {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Invalid USERS response" << std::endl;
    }
    
    return users;
}

bool Client::sendAUTH(const std::string& login, const std::string& password) {
    char buffer[BUFFER_SIZE];
    std::string response;
    std::string messageToServer = "AUTH " + login + " " + password;
    std::cout << "Ввели логин и пароль " << login << " " << password << std::endl; 
    std::cout << messageToServer << std::endl;
    
    if (send(clientSock, messageToServer.c_str(), messageToServer.length(), 0) < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error send AUTH message to server" << std::endl;
        return false;
    }

    while (true)
    {
        int bytesRead = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cerr << "Error receiving AUTH response" << std::endl;
            return false;
        }
        buffer[bytesRead] = '\0';
        response += buffer;
        //if (response.find('\n') != std::string::npos) break;

        response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
        response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());

        if (response == "AUTH_SUCCESS") {
            logInfo("Аутентификация прошла успешно для " + login);
            std::cout << "Аутентификация прошла успешно для " << login << std::endl;
            startThread();
            return true;
        } else if (response == "AUTH_FAILED") {
            logError("Ошибка аутентификации для " + login);
            std::cout << "Ошибка аутентификации для " << login << std::endl;
            return false;
        } else {
            logError("Неизвестный ответ от сервера: " + response);
            std::cout << "Неизвестный ответ от сервера: " << response << std::endl;
            return false;
        }
        return true;
    }
    
    
    /*int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);  
    if (bytesRead <= 0) {  
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error to connect server or breaking the connection";
        return false;
    }
    buffer[bytesRead] = '\0';
    std::string responce(buffer);
    const std::string authOk = "AUTH_SUCCESS";

    if (responce.find(authOk, 0) == 0) {
        logInfo(std::string("Response server: ") + authOk);
        // If server sent additional data after AUTH_SUCCESS (e.g. history), print it now
        if (responce.size() > authOk.size()) {
            std::string rest = responce.substr(authOk.size());
            if (!rest.empty()) std::cout << rest << std::endl;
        }
        startThread();
        return true;
    } else if (responce == "AUTH_FAILED") {  
        logError("AUTH_FAILED response from server");
        return false; 
    } else {
        logError("Unknown response from server during AUTH");
        return false;
    }*/
}

bool Client::sendToAll(const std::string& message) {
    char buffer[BUFFER_SIZE];
    std::ostringstream oss;
    oss << "ALL " << message;
    std::string messageToAll = oss.str();
    if (send(clientSock, messageToAll.c_str(), messageToAll.length(), 0) < 0) {
        logError("Error sending message to all");
        return false;
    }
    return true;
}
void Client::sendPrivate(const std::string& sender, const std::string& receiver, const std::string& message)
{
    std::ostringstream ossPrivate;
    ossPrivate << "PRIVATE " << sender << " " << receiver << " " << message;
    std::string privateMessage = ossPrivate.str();
    char buffer[BUFFER_SIZE];

    if (send(clientSock, privateMessage.c_str(), privateMessage.length(), 0) < 0)
    {
        logError("Error send private message");
        return;
    } else {
        // success will be observed when the server forwards message back to clients
    }
}
void Client::receiveMessages() {
    char buffer[BUFFER_SIZE];

    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int rec = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

        if (rec <= 0) {
            // If we've already initiated disconnect, exit quietly
            if (!running) break;
            if (rec == 0) {
                logInfo("Server closed connection");
            } else {
                logError("Error reading the message");
            }
            disconnect();
            break;
        }

        incomingBuffer.append(buffer, rec);

        // extract full lines delimited by '\n'
        size_t pos;
        while ((pos = incomingBuffer.find('\n')) != std::string::npos) {
            std::string line = incomingBuffer.substr(0, pos);
            // remove possible '\r'
            if (!line.empty() && line.back() == '\r') line.pop_back();
            incomingBuffer.erase(0, pos + 1);

            // process the line
            if (waitingForHistory) {
                if (line == "END_OF_HISTORY") {
                    std::unique_lock<std::mutex> hlock(history_mutex);
                    waitingForHistory = false;
                    history_cv.notify_one();
                } else {
                    std::unique_lock<std::mutex> hlock(history_mutex);
                    historyLines.push_back(line);
                }
            } else if (waitingForPrivateHistory) {
                if (line == "END_OF_HISTORY") {
                    std::unique_lock<std::mutex> phlock(private_history_mutex);
                    waitingForPrivateHistory = false;
                    private_history_cv.notify_one();
                } else {
                    std::unique_lock<std::mutex> phlock(private_history_mutex);
                    privateHistoryLines.push_back(line);
                }
            } else if (waitingForUsers) {
                std::unique_lock<std::mutex> ulock(users_mutex);
                usersResponse = line;
                waitingForUsers = false;
                users_cv.notify_one();
            } else {
                // live message: print and add to recent buffer
                {
                    logInfo(line);
                }
                {
                    std::lock_guard<std::mutex> rlock(recent_mutex);
                    recentMessages.push_back(line);
                    while (recentMessages.size() > recentLimit) recentMessages.pop_front();
                }
            }
        }
    }
    
}

void Client::startThread() {
    if (!isThreadRunning)
    {
        std::thread(&Client::receiveMessages, this).detach();
        isThreadRunning = true;
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Message reader started" << std::endl;
        }
    }   
}

void Client::stopReceivedMessage() {
    running = false;
}

void Client::disconnect() {
    std::string exitMessage = "EXIT";

    if(clientSock != -1 && running)
    {
        if (send(clientSock, exitMessage.c_str(), exitMessage.length(), 0) < 0) {
            {
                std::lock_guard<std::mutex> lock(io_mutex);
                std::cerr << "Error sending exit message" << std::endl;
            }
        } else {
            {
                std::lock_guard<std::mutex> lock(io_mutex);
                std::cout << "The exit message was sent successfully" << std::endl;
            }
        }
    }  else {
        {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Client socket is not connected" << std::endl;
        }
    }
    

    #ifdef _WIN32
        if(clientSock != -1)
            closesocket(clientSock);
        clientSock = -1;
        WSACleanup();
    #else
        if(clientSock != -1)
            close(clientSock);
        clientSock = -1;
    #endif

    running = false;
    isThreadRunning = false;
    {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cout << "Client disconnected" << std::endl;
    }
}

void Client::requestHistory() {
    char buffer[BUFFER_SIZE];
    std::string req = "GET_HISTORY";
    // Prepare to receive history before sending to avoid race where server replies before flag set
    {
        std::unique_lock<std::mutex> hlock(history_mutex);
        historyLines.clear();
        waitingForHistory = true;
    }
    if (send(clientSock, req.c_str(), req.length(), 0) < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error sending GET_HISTORY message to server" << std::endl;
        // reset flag
        std::unique_lock<std::mutex> hlock(history_mutex);
        waitingForHistory = false;
        return;
    }

    // Wait until receiver notifies that END_OF_HISTORY arrived
    {
        std::unique_lock<std::mutex> hlock(history_mutex);
        history_cv.wait(hlock, [this]() { return !waitingForHistory; });
        // Print the collected history but filter out recent messages to avoid duplicates
        std::lock_guard<std::mutex> lock(io_mutex);
        if (historyLines.empty()) {
            std::cout << "(No history)" << std::endl;
        } else {
            for (const auto &line : historyLines) {
                bool found = false;
                {
                    std::lock_guard<std::mutex> rlock(recent_mutex);
                    for (const auto &r : recentMessages) {
                        if (r == line) { found = true; break; }
                    }
                }
                if (!found) std::cout << line << std::endl;
            }
        }
    }
}

void Client::requestPrivateHistory(const std::string &other) {
    std::string req = "GET_PRIVATE " + other;

    // Prepare to receive private history before sending
    {
        std::unique_lock<std::mutex> lock(private_history_mutex);
        privateHistoryLines.clear();
        waitingForPrivateHistory = true;
    }
    if (send(clientSock, req.c_str(), req.length(), 0) < 0) {
        std::lock_guard<std::mutex> lock(io_mutex);
        std::cerr << "Error sending GET_PRIVATE message to server" << std::endl;
        std::unique_lock<std::mutex> lock2(private_history_mutex);
        waitingForPrivateHistory = false;
        return;
    }

    // Wait until receiver notifies that END_OF_HISTORY arrived
    {
        std::unique_lock<std::mutex> lock(private_history_mutex);
        private_history_cv.wait(lock, [this]() { return !waitingForPrivateHistory; });
        // Print the collected private history
        std::lock_guard<std::mutex> iolock(io_mutex);
        if (privateHistoryLines.empty()) {
            std::cout << "(No private history)" << std::endl;
        } else {
            for (const auto &line : privateHistoryLines) std::cout << line << std::endl;
        }
    }
}