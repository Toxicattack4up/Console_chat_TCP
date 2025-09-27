//#include <Windows.h>
#include <iostream>
#include "Menu.h"
#include "Account.h"
#include "DB.h"
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
    //Client client;
    //Account account;
    //Menu menu;
    //client.connectToServer("127.0.0.1");
    //menu.RunMenu(client);

    DB db("tcp_chat.db");
    //db.getUserId("peter_scott");
    std::string login = "Giatler";
    std::string name = "Гитлер";
    std::string password = "1488";
    db.addUser(login, name, password);
    /*db.addUser("ivan_ivanov", "Gennadiy", "54321");
    db.addUser("maria_smirnova", "Мария Смирнова", "qwerty");
    db.addUser("alexey_petrov", "Алексей Петров", "password");
    db.addUser("elena_kuznetsova", "Елена Кузнецова", "letmein");*/



    return 0;
}