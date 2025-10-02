#include "Client.h"
#include "../chat_server_tcp/Server.h"
#include "Menu.h"
#include <iostream>

int main() {
    Client client;
    Menu menu;
    client.connectToServer("127.0.0.1");
    menu.RunMenu(client);

}