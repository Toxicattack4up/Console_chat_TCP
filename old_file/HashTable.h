#include <string>
#include <utility>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

class HashTable {
private:
    static const int TABLE_SIZE = 101; // Размер хэш-таблицы
    bool occupied[TABLE_SIZE]; // Массив для отслеживания занятых ячеек
    std::pair<std::string, std::string> table[TABLE_SIZE]; // Таблица для хранения пар (логин, хэш)
    int hashFunction(const std::string& key);
    int quadraticProbing(int hash, int attempt);

public:
    HashTable();
    void insert(const std::string& login, const std::string& hash);
    std::string find(const std::string& login);
    void remove(const std::string& login);
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);
};