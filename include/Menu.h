#pragma once
#include <string>
#include <vector>
#include "../src/chat_client_tcp/Client.h"

class Menu {
private:
    std::string current_user; // Текущий вошедший пользователь
    
    // Приватные методы для организации кода
    void displayMainMenu();
    void displayUserMenu();
    void displayPrivateChatHeader(const std::string& receiver);
    void displayPublicChatHeader();
    void displayHelpCommands(bool isPrivateChat = false);
    
    // Основные обработчики
    bool handleLogin(Client& client);
    bool handleRegistration(Client& client);
    void handlePrivateChat(Client& client);
    void handlePublicChat(Client& client);
    std::string selectUserFromList(const std::vector<std::string>& users);
    
    // Вспомогательные методы
    void processPrivateMessage(Client& client, const std::string& receiver, const std::string& message);
    void processPublicMessage(Client& client, const std::string& message);
    bool processCommand(Client& client, const std::string& command, const std::string& context = "");

public:
    // Основные публичные методы
    void ClearScreen();
    int RunMenu(Client& client);
    int UserMenu(Client& client);
    int ShowAllMessages(Client& client);
};