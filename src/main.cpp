//#include <Windows.h>
#include <iostream>
#include "Menu.h"
#include "chat_client_tcp/Client.h"
#include "chat_server_tcp/Server.h"

int main() {
    Server server;
    server.run();

    return 0;
}