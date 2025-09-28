//#include <Windows.h>
#include <iostream>
#include "Menu.h"
#include "chat_client_tcp/Client.h"
#include "chat_server_tcp/Server.h"
#include "chat_server_tcp/Database.h"

int main() {
   
    ChatDB db("tcp_chat.db");
    Client client;
    Menu menu;
    

    return 0;
}