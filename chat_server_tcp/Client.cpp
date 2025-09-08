
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
    #ifdef _WIN32
        if(clientSock != -1)
            closesocket(clientSock);
        WSACleanup();
    #else
        if(clientSock != -1)
            close(clientSock);
    #endif

    running = false;
    std::cout << "Client disconnected" << std::endl;
}

bool Client::sendRegister(const std::string& login, const std::string& password, const std::string& name) {
    
}

void Client::connectToServer(const std::string& ip_to_server)
{
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { std::cerr << "Winsock error"; exit(1); }
    #endif
    
    clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSock < 0){
        std::cerr << "Error to create socket" << std::endl;
        exit(1);
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    
    if(inet_pton(AF_INET, ip_to_server.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cerr << "Invalid IP address" << std::endl;
        exit(1);
    }
    serverAddress.sin_port = htons(serverPort);
    if (connect(clientSock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0)
    {
        std::cout << "Connected to server" << std::endl;
    } else {
        std::cerr << "Error to connect server" << std::endl;
        exit(1);
    }
}

void Client::isConnected() {

}

std::vector<std::string> Client::getListOfUsers() {

}

bool Client::sendAUTH(const std::string& login, const std::string& password) {
    char buffer[BUFFER_SIZE];
    std::string messageToServer = "AUTH " + login + " " + password;
    
    if (send(clientSock, messageToServer.c_str(), messageToServer.length(), 0) < 0)
    {
        std::cerr << "Error send AUTH message to server" << std::endl;
        return false;
    }
    
    int bytesRead = recv(clientSock, buffer, sizeof(buffer), 0);  
    if (bytesRead <= 0) 
    {  
        std::cerr << "Error to connect server or breaking the connection";
        return false;
    }
    buffer[bytesRead] = '\0';
    std::string responce(buffer);
    if (responce == "AUTH_SUCCESS")
    {
        std::cout << "Responce server: " << buffer << std::endl;
        startThread();
        return true;
    } else if (responce == "AUTH_FAILED") {  
        std::cerr << "Error responce\n" << std::endl;
        return false; 
    } else {
        std::cerr << "Error responce\n" << std::endl;
        return false;
    }
}

bool Client::sendToAll(const std::string& message) {
    char buffer[BUFFER_SIZE];
    std::ostringstream oss;
    oss << "ALL " << message;
    std::string messageToAll = oss.str();
    if (send(clientSock, messageToAll.c_str(), messageToAll.length(), 0) < 0)
    {
        std::cerr << "Error sending message to all" << std::endl;
        return false;
    } else {
        std::cout << "The message was sent successfully" << std::endl;
        return true;
    }
}
void Client::sendPrivate(const std::string& sender, const std::string& receiver, const std::string& message)
{
    std::ostringstream ossPrivate;
    ossPrivate << "PRIVATE " << sender << " " << receiver << " " << message;
    std::string privateMessage = ossPrivate.str();
    char buffer[1024];
    if (send(clientSock, privateMessage.c_str(), privateMessage.length(), 0) < 0)
    {
        std::cerr << "Error send private message" << std::endl;
        return;
    } else {
        std::cout << "The message was sent successfully" << std::endl;
    }
}
void Client::receiveMessages() {
    std::cout << "The message reading stream is running" << std::endl;
    char buffer[BUFFER_SIZE];
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int rec = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (rec <= 0) {
            std::cerr << "Error reading the message" << std::endl;
            disconnect();
            break;
        } else {
            buffer[rec] = '\0';
            std::cout << buffer << std::endl;
        }
    }
    
}

void Client::startThread() {
    if (!isThreadRunning)
    {
        std::thread(&Client::receiveMessages, this).detach();
        isThreadRunning = true;
        std::cout << "Starts thead" << std::endl;
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
            std::cerr << "Error sending exit message" << std::endl;
        } else {
            std::cout << "The exit message was sent successfully" << std::endl;
        }
    }  else {
        std::cout << "Client socket is not connected" << std::endl;  
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
    std::cout << "Client disconnected" << std::endl;
}