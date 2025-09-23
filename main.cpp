//#include <Windows.h>
#include <iostream>
#include "Menu.h"
#include "Account.h"
#include "chat_server_tcp/Client.h"
#include "chat_server_tcp/Server.h"


//Условия задачи, создать чат со следующими параметрами
//1. Консольная программа
//2. Регистрация пользователей - логин, пароль, имя
//3. Вход в чат по логину и паролю
//4. Отправка сообщений конкретному пользователю
//5. Обмен сообщениями между всеми пользователями чата одновременно

//Обязательно надо использовать классы

int main() {
    // Установка кодировки консоли на UTF-8
    //if (!SetConsoleOutputCP(CP_UTF8)) {
    //    std::cerr << "Ошибка: не удалось установить кодировку вывода на UTF-8." << std::endl;
    //    return 1;
    //}
    //if (!SetConsoleCP(CP_UTF8)) {
    //    std::cerr << "Ошибка: не удалось установить кодировку ввода на UTF-8." << std::endl;
    //    return 1;
    //}
    Client client;
    Account account;
    Menu menu;
    client.connectToServer("127.0.0.1");
    menu.RunMenu(client);

    return 0;
}