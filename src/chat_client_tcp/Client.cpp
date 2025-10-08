#include "Client.h"

Client::Client()
{
    clientSock = -1;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        exit(1);
    }
#endif
}

Client::~Client()
{
    logOut();
    disconnect();
    std::lock_guard<std::mutex> lock(io_mutex);
}
// Регистрация (Готов)
bool Client::sendRegister(const std::string &login, const std::string &username, const std::string &password)
{
    char buffer[BUFFER_SIZE];
    std::string registerMessageToServer = "REGISTER " + login + " " + username + " " + password;
    std::string response;

    if (registerMessageToServer.size() > BUFFER_SIZE - 50)
    {
        std::cerr << "REGISTER message is too long" << std::endl;
        return false;
    }

    // Обеспечиваем подключение при регистрации, если было закрыто после logout
    if (clientSock == -1)
    {
        if (!ensureConnected())
        {
            std::cerr << "Клиент не подключен" << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "Подключено к серверу" << std::endl;
    }

    if (send(clientSock, registerMessageToServer.c_str(), registerMessageToServer.length(), 0) < 0)
    {
        std::cerr << "Ошибка при отправке сообщения REGISTER на сервер" << std::endl;
        return false;
    }

    while (true)
    {
        int bytesRead = recv(clientSock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0)
        {
            std::cerr << "Error receiving AUTH response" << std::endl;
            std::cerr << "Ошибка при получении ответа AUTH" << std::endl;
            return false;
        }
        buffer[bytesRead] = '\0';
        response += buffer;
        if (response.find('\n') != std::string::npos)
            break;
    }

    response.erase(std::remove(response.begin(), response.end(), '\n'), response.end());
    response.erase(std::remove(response.begin(), response.end(), '\r'), response.end());

    if (response == "REGISTER_SUCCESS")
    {
        logInfo("Регистрация прошла успешно для " + login);
        std::cout << "Регистрация прошла успешно для " << login << std::endl;
        return true;
    }
    else if (response == "REGISTER_FAILED")
    {
        logError("Ошибка регистрации для " + login);
        std::cout << "Ошибка регистрации для " << login << std::endl;
        return false;
    }
    else
    {
        logError("Неизвестный ответ от сервера: " + response);
        std::cout << "Неизвестный ответ от сервера: " << response << std::endl;
        return false;
    }
}
// Покдлючение к серверу (Готов)
void Client::connectToServer(const std::string &ip_to_server)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "Winsock error";
        exit(1);
    }
#endif

    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock < 0)
    {
        std::cerr << "Error to create socket" << std::endl;
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;

    if (inet_pton(AF_INET, ip_to_server.c_str(), &serverAddress.sin_addr) <= 0)
    {
        std::cerr << "Invalid IP address" << std::endl;
        exit(1);
    }
    serverAddress.sin_port = htons(serverPort);
    serverIp = ip_to_server;
    if (connect(clientSock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0)
    {
    }
    else
    {
        logError("Error to connect server");
        exit(1);
    }

    // Устанавливаем таймаут чтения, чтобы поток чтения мог завершиться при logout
#ifdef _WIN32
    // Перевод сокета в неблокирующий режим и задание таймаута через select/WSA
    DWORD timeoutMs = 1000;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeoutMs, sizeof(timeoutMs));
#else
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

bool Client::ensureConnected()
{
    if (clientSock != -1)
        return true;
    if (serverIp.empty())
        return false;
    connectToServer(serverIp);
    return clientSock != -1;
}
// Проверка подключения (Готов)
void Client::isConnected()
{
    if (clientSock != -1 && running)
        std::cout << "Client is connected" << std::endl;
    else
        std::cout << "Client is not connected" << std::endl;
}
// Получение списка пользователей (Готов)
std::vector<std::string> Client::getListOfUsers()
{
    std::string getUsersMessage = "GET_USERS";
    std::vector<std::string> users;

    if (clientSock == -1)
    {
        std::cerr << "Клиент не подключен" << std::endl;
        return users;
    }

    usersResponse.clear();
    waitingForUsers = true;

    if (send(clientSock, getUsersMessage.c_str(), getUsersMessage.length(), 0) < 0)
    {
        std::cerr << "Ошибка отправки сообщения GET_USERS на сервер" << std::endl;
        return {};
    }

    std::unique_lock<std::mutex> ulock(users_mutex);
    if (!users_cv.wait_for(ulock, std::chrono::seconds(5), [this]()
                           { return !waitingForUsers; }))
    {
        std::cerr << "Timeout waiting for USERS response" << std::endl;
        return users;
    }
    // Разбор ответа
    if (usersResponse.find("USERS ") == 0)
    {
        std::string usersList = usersResponse.substr(6);
        std::istringstream iss(usersList);
        std::string user;
        while (iss >> user)
            users.push_back(user);

        return users;
    }
    else
        std::cerr << "Неверный ответ USERS" << std::endl;

    return users;
}
// Метод для отправки команды аутентификации на сервер (Готов)
bool Client::sendAUTH(const std::string &login, const std::string &password)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    std::string response;
    std::string messageToServer = "AUTH " + login + " " + password;
    // Авторизация

    if (clientSock == -1)
    {
        if (!ensureConnected())
        {
            std::cerr << "Клиент не подключен" << std::endl;
            return false;
        }
    }

    if (send(clientSock, messageToServer.c_str(), messageToServer.length(), 0) < 0)
    {
        std::cerr << "Ошибка отправки авторизационного сообщения на сервер" << std::endl;
        return false;
    }
    std::cout << "Выполняется вход..." << std::endl;
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead <= 0)
        {
            std::cerr << "Ошибка при получении ответа AUTH" << std::endl;
            return false;
        }
        buffer[bytesRead] = '\0';
        response += buffer;

        // Нормализуем ответ: берём первую строку без CRLF
        size_t nlPos = response.find('\n');
        std::string line = nlPos == std::string::npos ? response : response.substr(0, nlPos);
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        if (line == "AUTH_SUCCESS")
        {
            std::cout << "Успешный вход как '" << login << "'" << std::endl;
            return true;
        }
        else if (line.rfind("AUTH_FAILED", 0) == 0)
        {
            std::cerr << "Неверный логин или пароль" << std::endl;
            return false;
        }
        else
        {
            std::cerr << "Неожиданный ответ сервера при входе: " << line << std::endl;
            return false;
        }
    }
}

bool Client::sendToAll(const std::string &message)
{
    std::ostringstream oss;
    oss << "ALL " << message;
    std::string messageToAll = oss.str();
    if (send(clientSock, messageToAll.c_str(), messageToAll.length(), 0) < 0)
    {
        logError("Сообщение об ошибке при отправке всем");
        return false;
    }
    return true;
}

void Client::sendPrivate(const std::string &sender, const std::string &receiver, const std::string &message)
{
    std::ostringstream ossPrivate;
    ossPrivate << "PRIVATE " << sender << " " << receiver << " " << message;
    std::string privateMessage = ossPrivate.str();
    char buffer[BUFFER_SIZE];

    if (send(clientSock, privateMessage.c_str(), privateMessage.length(), 0) < 0)
    {
        logError("Ошибка при отправке личного сообщения");
        return;
    }
    else
    {
        // success will be observed when the server forwards message back to clients
        logInfo("Личное сообщение, отправленное от " + sender + " к " + receiver);
    }
}

void Client::receiveMessages()
{
    char buffer[BUFFER_SIZE];

    while (running)
    {
        memset(buffer, 0, sizeof(buffer));
        int rec = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

        if (rec <= 0)
        {
            if (!running)
                break;
            if (rec == 0)
            {
                std::cout << "Соединение с сервером закрыто" << std::endl;
                break;
            }
#ifdef _WIN32
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == WSAETIMEDOUT)
            {
                continue;
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
#endif
            std::cerr << "Ошибка чтения данных от сервера" << std::endl;
            break;
        }

        incomingBuffer.append(buffer, rec);

        size_t pos;
        while ((pos = incomingBuffer.find('\n')) != std::string::npos)
        {
            std::string line = incomingBuffer.substr(0, pos);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            incomingBuffer.erase(0, pos + 1);

            if (waitingForHistory)
            {
                if (line == "END_OF_HISTORY")
                {
                    waitingForHistory = false;
                    history_cv.notify_one();
                }
                else
                {
                    historyLines.push_back(line);
                }
            }
            else if (waitingForPrivateHistory)
            {
                if (line == "END_OF_HISTORY")
                {
                    waitingForPrivateHistory = false;
                    private_history_cv.notify_one();
                }
                else
                {
                    privateHistoryLines.push_back(line);
                }
            }
            else if (waitingForUsers)
            {
                usersResponse = line;
                waitingForUsers = false;
                users_cv.notify_one();
            }
            else
            {
                logInfo(line);
                recentMessages.push_back(line);
                while (recentMessages.size() > recentLimit)
                    recentMessages.pop_front();
            }
        }
    }
    // Поток чтения завершён
    isThreadRunning = false;
}

void Client::startThread()
{
    if (!isThreadRunning)
    {
        running = true;
        std::thread(&Client::receiveMessages, this).detach();
        isThreadRunning = true;
        std::cout << "Запущен процесс чтения сообщений" << std::endl;
    }
}

void Client::stopReceivedMessage()
{
    running = false;
}
// Отключение от сервера (Готов)
void Client::disconnect()
{
#ifdef _WIN32
    closesocket(clientSock);
    clientSock = -1;
    WSACleanup();
#else
    close(clientSock);
    clientSock = -1;
#endif
}
// Выход из системы
void Client::logOut()
{
    std::string exitMessage = "EXIT";
    if (clientSock != -1 && running)
    {
        if (send(clientSock, exitMessage.c_str(), exitMessage.length(), 0) < 0)
        {
            std::cerr << "Ошибка при отправке сообщения о выходе" << std::endl;
        }
        else
        {
            std::cout << "Сообщение о выходе было успешно отправлено" << std::endl;
        }
    }

    running = false;
    isThreadRunning = false;

    std::cout << "Клиент отключен" << std::endl;
}

void Client::requestHistory()
{
    char buffer[BUFFER_SIZE];
    std::string req = "GET_HISTORY";
    std::unique_lock<std::mutex> hlock(history_mutex);
    historyLines.clear();
    waitingForHistory = true;

    if (send(clientSock, req.c_str(), req.length(), 0) < 0)
    {
        std::cerr << "Ошибка при отправке сообщения GET_HISTORY на сервер" << std::endl;
        waitingForHistory = false;
        return;
    }

    history_cv.wait(hlock, [this]()
                    { return !waitingForHistory; });

    if (historyLines.empty())
    {
        std::cout << "(Нет истории сообщений)" << std::endl;
    }
    else
    {
        for (const auto &line : historyLines)
        {
            bool found = false;
            for (const auto &r : recentMessages)
            {
                if (r == line)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                std::cout << line << std::endl;
        }
    }
}

void Client::requestPrivateHistory(const std::string &other)
{
    std::string req = "GET_PRIVATE " + other;
    std::unique_lock<std::mutex> plock(private_history_mutex);
    privateHistoryLines.clear();
    waitingForPrivateHistory = true;

    if (send(clientSock, req.c_str(), req.length(), 0) < 0)
    {
        std::cerr << "Ошибка при отправке сообщения GET_PRIVATE на сервер" << std::endl;
        waitingForPrivateHistory = false;
        return;
    }

    private_history_cv.wait(plock, [this]()
                            { return !waitingForPrivateHistory; });
    if (privateHistoryLines.empty())
    {
        std::cout << "(Нет истории приватных сообщений)" << std::endl;
    }
    else
    {
        for (const auto &line : privateHistoryLines)
            std::cout << line << std::endl;
    }
}