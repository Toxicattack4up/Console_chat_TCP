#include <iostream>
#include "Server.h"

int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // Также устанавливаем локаль для стандартных функций C++
        setlocale(LC_ALL, "ru_RU.UTF-8");
    #endif

    
    Server server;
    server.run();
    return 0;
}
