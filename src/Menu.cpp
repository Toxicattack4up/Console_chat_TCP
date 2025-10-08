#include "Menu.h"


static std::string ReadLineLocked(const std::string &prompt = "")
{
    // std::lock_guard<std::mutex> lock(io_mutex);
    if (!prompt.empty())
    {
        std::cout << prompt;
    }

    std::string line;
    std::getline(std::cin, line);
    return line;
}

void Menu::ClearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Выводит главное меню приложения
void Menu::displayMainMenu()
{
    std::cout << "╔══════════════════════════╗" << std::endl;
    std::cout << "║      ДОБРО ПОЖАЛОВАТЬ    ║" << std::endl;
    std::cout << "╠══════════════════════════╣" << std::endl;
    std::cout << "║ 1. Вход                  ║" << std::endl;
    std::cout << "║ 2. Регистрация           ║" << std::endl;
    std::cout << "║ 3. Выход                 ║" << std::endl;
    std::cout << "╚══════════════════════════╝" << std::endl;
}

// Выводит меню пользователя после входа
void Menu::displayUserMenu()
{
    std::cout << "╔══════════════════════════╗" << std::endl;
    std::cout << "║    МЕНЮ ПОЛЬЗОВАТЕЛЯ     ║" << std::endl;
    std::cout << "╠══════════════════════════╣" << std::endl;
    std::cout << "║ Пользователь: " << std::left;
    std::cout.width(10);
    if (current_user.length() > 10)
    {
        std::cout << current_user.substr(0, 7) + "...";
    }
    else
    {
        std::cout << current_user;
    }
    std::cout << "║" << std::endl;
    std::cout << "╠══════════════════════════╣" << std::endl;
    std::cout << "║ 1. Приватный чат         ║" << std::endl;
    std::cout << "║ 2. Общий чат             ║" << std::endl;
    std::cout << "║ 3. История сообщений     ║" << std::endl;
    std::cout << "║ 4. Список пользователей  ║" << std::endl;
    std::cout << "║ 5. Выход                 ║" << std::endl;
    std::cout << "╚══════════════════════════╝" << std::endl;
}

// Заголовок приватного чата с указанным собеседником
void Menu::displayPrivateChatHeader(const std::string &receiver)
{
    ClearScreen();
    std::cout << "╔══════════════════════════╗" << std::endl;
    std::cout << "║     ПРИВАТНЫЙ ЧАТ        ║" << std::endl;
    std::cout << "╠══════════════════════════╣" << std::endl;
    std::cout << "║ Вы -> " << std::left;
    if (receiver.length() > 16)
    {
        std::cout.width(16);
        std::cout << receiver.substr(0, 13) + "...";
    }
    else
    {
        std::cout.width(16);
        std::cout << receiver;
    }
    std::cout << "║" << std::endl;
    std::cout << "╚══════════════════════════╝" << std::endl;
    std::cout << "Введите /help для списка команд" << std::endl;
    std::cout << "──────────────────────────────" << std::endl;
}

// Заголовок общего чата
void Menu::displayPublicChatHeader()
{
    ClearScreen();
    std::cout << "╔══════════════════════════╗" << std::endl;
    std::cout << "║       ОБЩИЙ ЧАТ          ║" << std::endl;
    std::cout << "╚══════════════════════════╝" << std::endl;
    std::cout << "Введите /help для списка команд" << std::endl;
    std::cout << "──────────────────────────────" << std::endl;
}

// Справка по доступным командам в чате
void Menu::displayHelpCommands(bool isPrivateChat)
{
    // std::lock_guard<std::mutex> lock(io_mutex);
    std::cout << "\n📋 КОМАНДЫ ЧАТА:" << std::endl;
    std::cout << "──────────────────────────────" << std::endl;

    if (isPrivateChat)
    {
        std::cout << "/exit    - вернуться в меню" << std::endl;
        std::cout << "/history - история переписки" << std::endl;
        std::cout << "/help    - показать эту справку" << std::endl;
    }
    else
    {
        std::cout << "/exit    - вернуться в меню" << std::endl;
        std::cout << "/history - общая история чата" << std::endl;
        std::cout << "/users   - список пользователей" << std::endl;
        std::cout << "/help    - показать эту справку" << std::endl;
        std::cout << "/quit    - выйти из приложения" << std::endl;
    }
    std::cout << "──────────────────────────────" << std::endl;
}

// Обработка авторизации пользователя
bool Menu::handleLogin(Client &client)
{
    std::string login = ReadLineLocked("Логин: ");
    if (login.empty())
    {
        std::cout << "Логин не может быть пустым!" << std::endl;
        return false;
    }

    std::string password = ReadLineLocked("Пароль: ");
    if (password.empty())
    {
        std::cout << "Пароль не может быть пустым!" << std::endl;
        return false;
    }

    if (client.sendAUTH(login, password) == true)
    {
        current_user = login;
        // Запускаем поток чтения после успешного входа
        client.startThread();
        // Разрешаем приёму сообщений
        // (running флаг устанавливается в Client::startThread)
        std::cout << "Успешный вход!" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Неверный логин или пароль!" << std::endl;
        return false;
    }
}

// Обработка регистрации нового пользователя
bool Menu::handleRegistration(Client &client)
{
    std::string login = ReadLineLocked("Введите логин: ");
    if (login.empty())
    {
        std::cout << "Логин не может быть пустым!" << std::endl;
        return false;
    }

    if (login.find(' ') != std::string::npos)
    {
        std::cout << "Логин не должен содержать пробелы!" << std::endl;
        return false;
    }

    std::string name = ReadLineLocked("Введите имя (или нажмите Enter, чтобы использовать логин): ");
    if (name.empty())
    {
        name = login; // Если имя не введено, используем логин
    }
    else if (name.find(' ') != std::string::npos)
    {
        std::cout << "Имя не должно содержать пробелы!" << std::endl;
        return false;
    }

    std::string password = ReadLineLocked("Введите пароль: ");
    if (password.empty())
    {
        std::cout << "Пароль не может быть пустым!" << std::endl;
        return false;
    }

    if (client.sendRegister(login, name, password))
    {
        std::cout << "Регистрация успешна!" << std::endl;
        return true;
    }
    else
    {
        std::cout << "Ошибка регистрации! Возможно, логин уже занят." << std::endl;
        return false;
    }
}

// Выбор собеседника из списка пользователей
std::string Menu::selectUserFromList(const std::vector<std::string> &users)
{
    std::vector<std::string> availableUsers;
    // Фильтруем пользователей, исключая текущего
    for (const auto &user : users)
    {
        if (user != current_user)
        {
            availableUsers.push_back(user);
        }
    }

    if (availableUsers.empty())
    {
        std::cout << "Нет доступных пользователей для чата!" << std::endl;
        return "";
    }

    std::cout << "\nДоступные пользователи:" << std::endl;
    std::cout << "──────────────────────────────" << std::endl;

    for (size_t i = 0; i < availableUsers.size(); ++i)
    {
        std::cout << " " << i + 1 << ". " << availableUsers[i] << std::endl;
    }

    std::string selection = ReadLineLocked("\nВыберите пользователя (номер или имя): ");
    if (selection.empty())
        return "";

    // Проверка выбора по номеру
    try
    {
        int idx = std::stoi(selection);
        if (idx >= 1 && idx <= static_cast<int>(availableUsers.size()))
        {
            return availableUsers[idx - 1];
        }
    }
    catch (...)
    {
        // Если не число, проверяем по имени
    }

    // Проверка по имени
    for (const auto &user : availableUsers)
    {
        if (user == selection)
        {
            return user;
        }
    }

    std::cout << "Пользователь не найден!" << std::endl;
    return "";
}

// Цикл приватного чата с выбранным пользователем
void Menu::handlePrivateChat(Client &client)
{
    auto users = client.getListOfUsers();
    if (users.empty())
    {
        std::cout << "Нет доступных пользователей!" << std::endl;
        ReadLineLocked("Нажмите Enter для продолжения...");
        return;
    }

    std::string receiver = selectUserFromList(users);
    if (receiver.empty())
        return;

    displayPrivateChatHeader(receiver);
    client.requestPrivateHistory(receiver);

    while (true)
    {
        std::string message = ReadLineLocked(">> ");

        if (message.empty())
            continue;

        if (message[0] == '/')
        {
            if (processCommand(client, message, receiver))
            {
                break;
            }
            continue;
        }
        processPrivateMessage(client, receiver, message);
    }
}

// Цикл общего чата
void Menu::handlePublicChat(Client &client)
{
    displayPublicChatHeader();
    client.requestHistory();

    while (true)
    {
        std::string message = ReadLineLocked(">> ");

        if (message.empty())
            continue;

        if (message[0] == '/')
        {
            if (processCommand(client, message))
            {
                break;
            }
            continue;
        }

        processPublicMessage(client, message);
    }
}

// Отправка приватного сообщения
void Menu::processPrivateMessage(Client &client, const std::string &receiver, const std::string &message)
{
    client.sendPrivate(current_user, receiver, message);
}

// Отправка сообщения в общий чат
void Menu::processPublicMessage(Client &client, const std::string &message)
{
    client.sendToAll(message);
}

// Обработка команд /exit, /help, /history, /users, /quit
bool Menu::processCommand(Client &client, const std::string &command, const std::string &context)
{
    if (command == "/exit")
    {
        return true;
    }
    else if (command == "/help")
    {
        displayHelpCommands(!context.empty());
    }
    else if (command == "/history")
    {
        if (context.empty())
        {
            client.requestHistory();
        }
        else
        {
            client.requestPrivateHistory(context);
        }
    }
    else if (command == "/users" && context.empty())
    {
        auto users = client.getListOfUsers();
        std::cout << "\nОнлайн пользователи:" << std::endl;
        for (const auto &user : users)
        {
            std::cout << " • " << user << std::endl;
        }
    }
    else if (command == "/quit" && context.empty())
    {
        client.disconnect();
        return true;
    }
    else
    {
        std::cout << "Неизвестная команда: " << command << std::endl;
    }
    return false;
}

// Главный цикл: показ главного меню
int Menu::RunMenu(Client &client)
{
    int choice = 0;
    RunMenubool = false;

    do
    {
        ClearScreen();
        displayMainMenu();

        std::string input = ReadLineLocked("Ваш выбор: ");
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            choice = -1;
        }

        switch (choice)
        {
        case 1:
            if (handleLogin(client))
            {
                UserMenu(client);
            }
            else
            {
                ReadLineLocked("Нажмите Enter для продолжения...");
            }
            break;
        case 2:
            if (handleRegistration(client) == true)
            {
                std::cout << "Регистрация успешна! Теперь войдите в систему." << std::endl;
            }
            else
            {
                std::cout << "Ошибка регистрации! Попробуйте снова." << std::endl;
            }
            ReadLineLocked("Нажмите Enter для продолжения...");
            break;
        case 3:
            RunMenubool = true;
            client.disconnect();
            break;
        default:
            std::cout << "Неверный выбор!" << std::endl;
            ReadLineLocked("Нажмите Enter для продолжения...");
            break;
        }
    } while (!RunMenubool);
    return 0;
}

// Цикл меню пользователя (после авторизации)
int Menu::UserMenu(Client &client)
{
    int choice = 0;
    bool shouldExit = false;
    RunMenubool = true;

    do
    {
        ClearScreen();
        displayUserMenu();

        std::string input = ReadLineLocked("Ваш выбор: ");
        try
        {
            choice = std::stoi(input);
        }
        catch (...)
        {
            choice = -1;
        }

        switch (choice)
        {
        case 1:
            handlePrivateChat(client);
            break;
        case 2:
            handlePublicChat(client);
            break;
        case 3:
            client.requestHistory();
            ReadLineLocked("\nНажмите Enter для возврата в меню...");
            break;
        case 4:
        {
            auto users = client.getListOfUsers();
            std::cout << "\nСписок пользователей:" << std::endl;
            for (const auto &user : users)
            {
                std::cout << " • " << user << std::endl;
            }
            ReadLineLocked("\nНажмите Enter для возврата в меню...");
        }
        break;
        case 5:
            client.logOut();
            RunMenubool = false;
            current_user.clear();
            shouldExit = true;
            std::cout << "\nВы вышли из аккаунта. Возврат в главное меню..." << std::endl;
            break;
        default:
            std::cout << "Неверный выбор. Введите число от 1 до 5." << std::endl;
            ReadLineLocked("Нажмите Enter, чтобы повторить ввод...");
            break;
        }
    } while (!shouldExit);

    return 0;
}

// Утилита: показать все сообщения общего чата
int Menu::ShowAllMessages(Client &client)
{
    handlePublicChat(client);
    return 0;
}