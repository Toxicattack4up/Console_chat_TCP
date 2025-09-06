
#include "Client.h"

class Client {
private:
    int clientSock;
    int serverPort = 12345;
    bool isThreadRunning = false;
    bool running = true;
    static const int max_message_size = 1024;
    
public:
    void connectToServer(const std::string& ip_to_server)
    {
        #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { std::cerr << "Winsock error"; exit(1); }
        #endif
        
        clientSock = socket(AF_INET, SOCK_STREAM, 0);
        if(clientSock < 0)
        {
            std::cerr << "Error to create socket" << std::endl;
            exit(1);
        }

        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        if(inet_pton(AF_INET, ip_to_server.c_str(), &serverAddress.sin_addr) <= 0)
        {
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

    bool sendAUTH(std::string& login, std::string& password)
    {
        char buffer[max_message_size];
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

    bool sendToAll(const std::string& message)
    {
        char buffer[1024];

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

    void sendPrivate(const std::string& sender, const std::string& receiver, const std::string& message)
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

    void receiveMessages()
    {
        std::cout << "The message reading stream is running" << std::endl;
        char buffer[2048];
        
        while (running)
        {
            memset(buffer, 0, sizeof(buffer));
            int rec = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

            if (rec <= 0)
            {
                std::cerr << "Error reading the message" << std::endl;
                disconnect();
                break;
            } else {
                buffer[rec] = '\0';
                std::cout << buffer << std::endl;
            }
        }
        
    }

    void startThread()
    {
        if (!isThreadRunning)
        {
            std::thread(&Client::receiveMessages, this).detach();
            isThreadRunning = true;
            std::cout << "Starts thead" << std::endl;
        }   
    }

    void stopReceivedMessage()
    {
        running = false;
    }

    void disconnect()
    {
        std::cout << "Disconnect server" << std::endl;
        stopReceivedMessage();
        close(clientSock);
    }

    void exitToProgramm()
    {
        std::cout << "Exit to programm" << std::endl;
        disconnect();
        exit(0);
    }
};
