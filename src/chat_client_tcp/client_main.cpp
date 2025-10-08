#include <iostream>
#include "Client.h"
#include "Menu.h"

int main()
{
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Также устанавливаем локаль для стандартных функций C++
    setlocale(LC_ALL, "ru_RU.UTF-8");
    #endif
    
    Client client;
    Menu menu;
    
    client.connectToServer("127.0.0.1");
    menu.RunMenu(client);
}