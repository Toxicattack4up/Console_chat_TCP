# ConsoleChat (TCP)

Многопоточный TCP-чат (клиент/сервер) на C++17 с хранением данных в SQLite и хешированием паролей через libsodium.

## Возможности

- Регистрация и авторизация пользователей
- Общий чат (broadcast) и приватные сообщения
- История сообщений (общая и приватная)
- Список пользователей
- Многопоточная обработка подключений
- Логирование действий сервера (без текста сообщений)

## Архитектура

```
src/
├── chat_server_tcp/   # Server, Database, server_main
├── chat_client_tcp/   # Client, client_main
├── Menu.cpp           # Консольное меню клиента
└── common.cpp
include/               # Общие заголовки
docs/architecture.drawio
tests/                 # GoogleTest для ChatDB
```

## Зависимости

- C++17, CMake ≥ 3.15
- SQLite3 (`libsqlite3-dev` / `brew install sqlite`)
- libsodium (`libsodium-dev` / `brew install libsodium`)

## Сборка

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

На Windows укажите toolchain vcpkg при необходимости (`sqlite3`, `libsodium`).

## Запуск

```bash
# терминал 1
./build/tcp_chat_server

# терминал 2
./build/tcp_chat_client
```

При первом запуске создаётся `tcp_chat.db` и пользователь `admin` / `admin123` (демо).

**Важно:** пароли хранятся как argon2id (`crypto_pwhash_str`). Старые записи с `std::hash` больше не принимаются — перерегистрируйте пользователей или удалите `tcp_chat.db`.

## Протокол (текстовые команды, строка + `\\n`)

| Команда | Описание |
| --- | --- |
| `REGISTER <login> <username> <password>` | Регистрация |
| `AUTH <login> <password>` | Вход |
| `ALL <text>` | Сообщение в общий чат |
| `PRIVATE <user> <text>` | Личное сообщение |
| `GET_USERS` | Список логинов |
| `GET_HISTORY` | История общего чата (заканчивается `END_OF_HISTORY`) |
| `GET_PRIVATE <user>` | История переписки |
| `EXIT` | Выход из сессии |

Ответы: `REGISTER_SUCCESS` / `REGISTER_FAILED: …`, `AUTH_SUCCESS` / `AUTH_FAILED: …`.

## Сетевая конфигурация

- Сервер: `0.0.0.0:12345`
- Клиент по умолчанию: `127.0.0.1:12345`

## Ограничения

- Нет TLS; подходит как учебный пример, не для продакшена
- Нет rate-limit и сложной политики паролей
- TCP framing на сервере — буфер строк по `\\n`; клиент должен слать строки с переводом строки

## Чему научился

- Сокеты TCP, потоки и синхронизация без дедлоков на mutex
- SQLite prepared statements
- Современное хранение паролей (libsodium)
- CMake + GoogleTest + CI

## Лицензия

MIT — см. [LICENSE](LICENSE).
