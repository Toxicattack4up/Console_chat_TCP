#pragma once
#include <string>
#include "Account.h"
#include "chat_server_tcp/Client.h"

// Класс для управления меню пользователя
class Menu
{
private:
    std::string current_user;
    Account account;
public:
    // Метод для очистки экрана и чтобы был красивый вывод
    void ClearScreen();

    // Метод для запуска основного меню
    int RunMenu(Client& client);

    // Меню для авторизованного пользователя
    int UserMenu(Client& client);

    // Метод для отображения всех сообщений
    int ShowAllMessages(Client& client);
};