#include "Menu.h"
#include "Chat.h"
#include <iostream>
#include <string>
#include <limits>
#include "common.h"

static std::string ReadLineLocked(const std::string &prompt = "") {
    std::lock_guard<std::mutex> lock(io_mutex);
    if (!prompt.empty()) std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void Menu::ClearScreen()
{
    std::cout << std::string(50, '\n');
}

int Menu::RunMenu(Client& client)
{
    int choice;
    bool shouldExit = false;
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
        {
            std::string line = ReadLineLocked("Меню: ");
            try { choice = std::stoi(line); } catch (...) { choice = -1; }
        }
        switch (choice)
        {
        case 1:
                login = ReadLineLocked("Введите логин: ");
            password = ReadLineLocked("Введите Пароль: ");

            if(client.sendAUTH(login, password) == true)
            {
                current_user = login;
                UserMenu(client);
            } else {
                std::lock_guard<std::mutex> lock(io_mutex);
                std::cerr << "Вы ввели неверный логин или пароль, попробуйте снова" << std::endl;
            }
            continue;
        case 2:
            account.Registration();
            break;
        case 3:
            shouldExit = true;
            break;
        default:
            std::cout << "Недопустимый выбор! Попробуйте снова." << std::endl;
            break;
        }
    } while (!shouldExit);
    return 0;
}

int Menu::UserMenu(Client& client)
{
    int choice = 0;
    bool shouldExit = false;
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
        {
            std::string line = ReadLineLocked("Ваш выбор: ");
            try { choice = std::stoi(line); } catch (...) { choice = -1; }
        }
        
        switch (choice)
        {
        case 1:
            {
                auto users = client.getListOfUsers();
                if (users.empty()) {
                    std::lock_guard<std::mutex> lock(io_mutex);
                    std::cout << "Нет доступных пользователей для приватного чата." << std::endl;
                    break;
                }

                std::lock_guard<std::mutex> lock(io_mutex);
                std::cout << "Доступные пользователи:" << std::endl;
                
                for (size_t i = 0; i < users.size(); ++i) {
                    std::cout << i+1 << ". " << users[i] << std::endl;
                }

                std::string selection = ReadLineLocked("Выберите пользователя по номеру или введите имя: ");
                if (selection.empty()) {
                    break;
                }

                std::string chosen;
                try {
                    int idx = std::stoi(selection);
                    if (idx >= 1 && idx <= (int)users.size()) chosen = users[idx-1];
                } catch (...) {
                    chosen = selection;
                }

                if (chosen.empty()) {
                    std::lock_guard<std::mutex> lock(io_mutex);
                    std::cout << "Неверный выбор пользователя." << std::endl;
                    break;
                }

                
                std::lock_guard<std::mutex> lock(io_mutex);
                std::cout << "Открыт приватный чат с: " << chosen << ". Введите /exit чтобы вернуться." << std::endl;
                
                while (true) {
                    std::string pm = ReadLineLocked("(private) ");
                    if (pm.empty()) continue;
                    
                    if (pm[0] == '/') {
                        if (pm == "/exit") break;
                        if (pm == "/help") {
                            std::lock_guard<std::mutex> lock(io_mutex);
                            std::cout << "Приватный чат команды: /exit - вернуться, /help - помощь, /history или h - показать приватную историю" << std::endl;
                            continue;
                        }
                        if (pm == "/history" || pm == "h") {
                            client.requestPrivateHistory(chosen);
                            ReadLineLocked("Нажмите Enter для продолжения...");
                            continue;
                        }
                        // Unknown slash command: inform user and do not send
                        {
                            std::lock_guard<std::mutex> lock(io_mutex);
                            std::cout << "Неизвестная команда: " << pm << std::endl;
                        }
                        continue;
                    }
                    // regular private message
                    client.sendPrivate(current_user, chosen, pm);
                }
            }
            break;
        case 2:
            ShowAllMessages(client);
            break;
        case 3:
            client.requestHistory();
            continue;
        case 4:
            client.disconnect();
            shouldExit = true;
            break;
        default:
            std::cout << "Недопустимый выбор! Попробуйте снова." << std::endl;
            break;
        }
    } while (!shouldExit);
    return 0;
}

int Menu::ShowAllMessages(Client& client)
{
    // Interactive public chat loop. Commands:
    // /exit       - return to previous menu
    // /history    - request and show public history
    // /all <text> - send <text> to everyone
    // /quit       - disconnect client and exit
    std::string line;
    // Enter public chat: show header and load history once
    ClearScreen();
    std::cout << "==========================" << std::endl;
    std::cout << "Общий чат (введите /help для списка команд):" << std::endl;
    // request public history once when entering the chat
    client.requestHistory();
    std::cout << "==========================" << std::endl;
    line = "";
    while (true) {
        line = ReadLineLocked("(chat) ");
        if (line.empty()) continue;
        if (line == "/exit") {
            return 0; // back to UserMenu
        }
        if (line == "/help") {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Команды: /exit /history /all <msg> /quit" << std::endl;
            continue;
        }
        if (line == "/history") {
            client.requestHistory();
            continue;
        }
        if (line.rfind("/all ", 0) == 0) {
            std::string msg = line.substr(5);
            if (!msg.empty()) client.sendToAll(msg);
            continue;
        }
        if (line == "/quit") {
            client.disconnect();
            return 0;
        }
        // If user types plain text, send to all
        if (!line.empty() && line[0] == '/') {
            std::lock_guard<std::mutex> lock(io_mutex);
            std::cout << "Неизвестная команда: " << line << std::endl;
            continue;
        }
        client.sendToAll(line);
    }
}
