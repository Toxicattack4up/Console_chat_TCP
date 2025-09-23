#include "Account.h"
#include <iostream>
#include <string>

Account::Account() : login(""), password(""), name("") {
}

Account::~Account() {
}

void Account::Registration() {
    std::cout << "=== РЕГИСТРАЦИЯ ===" << std::endl;
    
    std::cout << "Введите логин: ";
    std::getline(std::cin, login);
    
    std::cout << "Введите пароль: ";
    std::getline(std::cin, password);
    
    std::cout << "Введите ваше имя: ";
    std::getline(std::cin, name);
    
    std::cout << "Регистрация завершена!" << std::endl;
    std::cout << "Логин: " << login << std::endl;
    std::cout << "Имя: " << name << std::endl;
}

// Геттеры
std::string Account::getLogin() const {
    return login;
}

std::string Account::getPassword() const {
    return password;
}

std::string Account::getName() const {
    return name;
}

// Сеттеры
void Account::setLogin(const std::string& login) {
    this->login = login;
}

void Account::setPassword(const std::string& password) {
    this->password = password;
}

void Account::setName(const std::string& name) {
    this->name = name;
}

