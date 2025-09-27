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
    // Диалог 1: kek ↔ john_doe
    /*db.addMessage("kek", "john_doe", "Привет! Как дела?");
    db.addMessage("john_doe", "kek", "Привет! Все отлично, спасибо!");
    db.addMessage("kek", "john_doe", "Что нового?");
    db.addMessage("john_doe", "kek", "Работаю над проектом, ты как?");

    // Диалог 2: jane_smith ↔ mike_wilson  
    db.addMessage("jane_smith", "mike_wilson", "Привет, Майк! Готов к встрече?");
    db.addMessage("mike_wilson", "jane_smith", "Да, готов! В 15:00?");
    db.addMessage("jane_smith", "mike_wilson", "Идеально! Увидимся!");

    // Диалог 3: sara_jones ↔ alex_brown
    db.addMessage("sara_jones", "alex_brown", "Алекс, поможешь с кодом?");
    db.addMessage("alex_brown", "sara_jones", "Конечно! В чем проблема?");
    db.addMessage("sara_jones", "alex_brown", "SQL запрос не работает...");

    // Диалог 4: lisa_davis ↔ tom_collins
    db.addMessage("lisa_davis", "tom_collins", "Том, документы готовы?");
    db.addMessage("tom_collins", "lisa_davis", "Да, забирай в любое время");

    // Диалог 5: emily_clark ↔ david_miller
    db.addMessage("emily_clark", "david_miller", "Дэвид, проверь почту");
    db.addMessage("david_miller", "emily_clark", "Проверил, ответил!");*/
    /*db.addUser("ivan_ivanov", "Gennadiy", "54321");
    db.addUser("maria_smirnova", "Мария Смирнова", "qwerty");
    db.addUser("alexey_petrov", "Алексей Петров", "password");
    db.addUser("elena_kuznetsova", "Елена Кузнецова", "letmein");*/

    std::vector<std::string> message = db.getUserMessages("kek");

    std::vector<std::string> messages = db.getMessages("kek", "john_doe");
    for (auto i : message) {
        std::cout << i << std::endl;
    }
    
    for (auto i : messages) {
        std::cout << i << std::endl;
    }

    return 0;
}