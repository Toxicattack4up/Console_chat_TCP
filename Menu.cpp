#include "Menu.h"
#include "Chat.h"
#include <iostream>
#include <string>
#include <limits>

void Menu::ClearScreen()
{
    std::cout << std::string(50, '\n');
}

int Menu::RunMenu(Client& client)
{
    int choice;
    bool exit = false;
    std::string login = "";
    std::string password = "";
    
    do
    {
        ClearScreen();
        std::cout << "==========================" << std::endl;
        std::cout << "Добро пожаловать!" << std::endl;
        std::cout << "1. Вход" << std::endl;
        std::cout << "2. Регистрация" << std::endl;
        std::cout << "3. Выход" << std::endl;
        std::cout << "==========================" << std::endl;
        std::cout << "Меню: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера ввода
        switch (choice)
        {
        case 1:
            std::cout << "Введите логин: " << std::endl;
            std::getline(std::cin,login);
            std::cout << "Введите Пароль: " << std::endl;
            std::getline(std::cin,password);

            if(client.sendAUTH(login, password) == true)
            {
                current_user = login;
                UserMenu(client);
            }else{
                std::cerr << "Вы ввели неверный логин или пароль, попробуйте снова" << std::endl;
            }
            continue;
        case 2:
            account.Registration();
            break;
        case 3:
            exit = true;
            break;
        default:
            std::cout << "Недопустимый выбор! Попробуйте снова." << std::endl;
            break;
        }
    } while (!exit);
    return 0;
}

int Menu::UserMenu(Client& client)
{
    int choice = 0;
    std::string receiver, message;
    
    ClearScreen();
    do
    {
        std::cout << "==========================" << std::endl;
        std::cout << "Меню пользователя:" << std::endl;
        std::cout << "1. Отправка сообщения пользователю" << std::endl;
        std::cout << "2. Сообщения для всех пользователей" << std::endl;
        std::cout << "3. Обновление чата" << std::endl;
        std::cout << "4. Выход" << std::endl;
        std::cout << "==========================" << std::endl;
        std::cout << "Ваш выбор: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очистка буфера ввода
        
        switch (choice)
        {
        case 1:
            std::cout << "Введите имя пользователя, которому хотите отправить сообщение: ";
            std::getline(std::cin, receiver);
            std::cout << "Введите сообщение: ";
            std::getline(std::cin, message);
            client.sendPrivate(current_user, receiver, message);
            break;
        case 2:
            ShowAllMessages(client);
            break;
        case 3:
            client.receiveMessages();
            continue;
        case 4:
            client.disconnect();
            RunMenu(client);
            break;
        default:
            std::cout << "Недопустимый выбор! Попробуйте снова." << std::endl;
            break;
        }
    } while (!exit);
    return 0;
}

int Menu::ShowAllMessages(Client& client)
{
    int choice = 0;
    Chat chat;
    std::string message;
    do
    {
        ClearScreen();
        std::cout << "==========================" << std::endl;
        std::cout << "Общий чат:" << std::endl;
        chat.PrintAllMessages();
        std::cout << "==========================" << std::endl;
        std::cout << "Меню:" << std::endl;
        std::cout << "1. Написать всем сообщение" << std::endl;
        std::cout << "2. Вернуться в предыдущее меню" << std::endl;
        std::cout << "3. Выход из приложения" << std::endl;
        std::cout << "Ваш выбор: ";
        std::cin >> choice;

        switch (choice)
        {
        case 1:
            std::cout << "Введите сообщение для всех пользователей: ";
            std::getline(std::cin, message);
            client.sendToAll(message);
            break;
        case 2:
            UserMenu(client);
            break;
        case 3:
            client.disconnect();
            break;
        default:
            std::cout << "Недопустимый выбор! Попробуйте снова." << std::endl;
            break;
        }
    } while (!exit);
    return 0;
}