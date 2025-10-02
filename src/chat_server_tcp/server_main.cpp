#include "Server.h"
#include "../chat_client_tcp/Client.h"
#include "Menu.h"
#include <iostream>

int main() {
    Server server;
    server.run();
    return 0;
}
