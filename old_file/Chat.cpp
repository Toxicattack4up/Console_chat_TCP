#include "Chat.h"

Chat::Chat() {
}

Chat::~Chat() {
}

void Chat::addMessage(const std::string& message) {
    messages.push_back(message);
}

void Chat::PrintAllMessages() {
    if (messages.empty()) {
        std::cout << "Нет сообщений в чате." << std::endl;
        return;
    }
    
    for (const auto& message : messages) {
        std::cout << message << std::endl;
    }
}

size_t Chat::getMessageCount() const {
    return messages.size();
}

