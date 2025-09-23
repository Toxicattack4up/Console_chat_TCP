#pragma once
#include <string>
#include <vector>

class Chat {
private:
    std::vector<std::string> messages;

public:
    Chat();
    ~Chat();
    
    // Метод для добавления сообщения
    void addMessage(const std::string& message);
    
    // Метод для отображения всех сообщений
    void PrintAllMessages();
    
    // Метод для получения количества сообщений
    size_t getMessageCount() const;
};

