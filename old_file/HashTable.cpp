#include "HashTable.h"

namespace fs = std::filesystem;

// Функция хэширования
int HashTable::hashFunction(const std::string& key) {
    double A = 0.6180339887;
    unsigned long long sum = 0;
    for (char c : key) {
        sum += static_cast<unsigned long long>(c);
    }
    double fracPart = sum * A - static_cast<int>(sum * A);
    return static_cast<int>(TABLE_SIZE * fracPart);
}

// Метод квадратичного пробирования
int HashTable::quadraticProbing(int hash, int attempt) {
    int c1 = 1;
    int c2 = 3;
    return static_cast<int>((static_cast<unsigned int>(hash) + c1 * attempt + c2 * attempt * attempt) % TABLE_SIZE);
}

HashTable::HashTable() {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        occupied[i] = false;
    }
}

// Вставка пары (логин, хэш)
void HashTable::insert(const std::string& login, const std::string& hash) {
    int hashValue = hashFunction(login);
    int attempt = 0;
    while (occupied[hashValue]) {
        if (table[hashValue].first == login) {
            // Логин уже существует, обновляем хэш
            table[hashValue].second = hash;
            return;
        }
        hashValue = quadraticProbing(hashValue, ++attempt);
        if (attempt >= TABLE_SIZE) {
            std::cerr << "Ошибка: хэш-таблица заполнена!" << std::endl;
            return;
        }
    }
    table[hashValue] = { login, hash };
    occupied[hashValue] = true;
}

// Поиск хэша по логину
std::string HashTable::find(const std::string& login) {
    int hashValue = hashFunction(login);
    int attempt = 0;
    while (occupied[hashValue]) {
        if (table[hashValue].first == login) {
            return table[hashValue].second;
        }
        hashValue = quadraticProbing(hashValue, ++attempt);
        if (attempt >= TABLE_SIZE) {
            break;
        }
    }
    return ""; // Логин не найден
}

// Удаление пары по логину
void HashTable::remove(const std::string& login) {
    int hashValue = hashFunction(login);
    int attempt = 0;
    while (occupied[hashValue]) {
        if (table[hashValue].first == login) {
            table[hashValue] = { "", "" };
            occupied[hashValue] = false;
            return;
        }
        hashValue = quadraticProbing(hashValue, ++attempt);
        if (attempt >= TABLE_SIZE) {
            break;
        }
    }
}

// Сохранение хэш-таблицы в файл
void HashTable::saveToFile(const std::string& filename) const {
    if (!fs::exists(filename)) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Ошибка: не удалось создать файл " << filename << "." << std::endl;
            return;
        }
    }
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << " для записи." << std::endl;
        return;
    }
    for (int i = 0; i < TABLE_SIZE; ++i) {
        if (occupied[i]) {
            file << table[i].first << " " << table[i].second << std::endl;
        }
    }
}

// Загрузка хэш-таблицы из файла
void HashTable::loadFromFile(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Ошибка: не удалось создать файл " << filename << "." << std::endl;
            return;
        }
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл " << filename << " для чтения." << std::endl;
        return;
    }
    std::string login, hash;
    while (file >> login >> hash) {
        insert(login, hash);
    }
}