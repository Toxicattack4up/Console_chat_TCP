//#include <Windows.h>
#include <iostream>
#include "Menu.h"
#include "chat_client_tcp/Client.h"

int main() {
    Client client;
    Menu menu;
    menu.RunMenu(client);
    return 0;
}