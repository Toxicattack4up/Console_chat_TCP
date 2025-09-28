#pragma once
#include <string>
#include <iostream>

class Account {
private:
    std::string login;
    std::string password;
    std::string name;

public:
    Account();
    ~Account();
    
    // Метод для регистрации нового пользователя
    void Registration();
    
    // Геттеры для получения данных пользователя
    std::string getLogin() const;
    std::string getPassword() const;
    std::string getName() const;
    
    // Сеттеры для установки данных пользователя
    void setLogin(const std::string& login);
    void setPassword(const std::string& password);
    void setName(const std::string& name);
};

