#include "Client.h"
#include "Server.h"
#include <iostream>

int main() {
    std::string login, password;
    Client client;
    client.connectToServer("127.0.0.1");
    std::cout << "Введите логин, а затем пароль" << std::endl; 
    std::cin >> login;
    std::cin >> password;

    client.sendAUTH(login, password);

}