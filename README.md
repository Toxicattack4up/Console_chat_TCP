# ConsoleChat (TCP)

Небольшой кроссплатформенный консольный чат (клиент/сервер) на C++ с хранением данных в SQLite.

- Клиент: консольное меню, приватные/общие сообщения, история, список пользователей
- Сервер: многопоточная обработка TCP‑соединений, маршрутизация сообщений, SQLite‑логирование
- Платформы: macOS, Linux, Windows (MSVC/MinGW)

## Возможности

- Регистрация и авторизация пользователей
- Общий чат (broadcast) и приватные сообщения
- История сообщений (общая и приватная)
- Список пользователей
- Информативные сообщения в консоли клиента и сервера

## Архитектура

- `src/chat_server_tcp/` — сервер (`Server.{h,cpp}`, `Database.{h,cpp}`, `server_main.cpp`)
- `src/chat_client_tcp/` — клиент (`Client.{h,cpp}`, `client_main.cpp`)
- `src/` — меню клиента (`Menu.cpp`) и общие утилиты (`common.*`)
- `include/` — общие заголовки
- `CMakeLists.txt` — корневой CMake (SQLite и ws2_32 для Windows)

## Зависимости

- C++17+
- CMake 3.10+
- SQLite3 (dev‑пакет)
  - macOS: `brew install sqlite`
  - Ubuntu/Debian: `sudo apt install libsqlite3-dev`
  - Windows (MSVC): `vcpkg install sqlite3` (или другая установка с CMake config)

## Сборка

### macOS / Linux
```bash
# 1) Клонирование
git clone <repo-url>
cd ConsoleChat

# 2) Генерация
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 3) Сборка
cmake --build build --config Release
```

### Windows (MSVC + vcpkg)
```bash
# Установите vcpkg и sqlite3
vcpkg install sqlite3

# Генерация проекта
cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE="<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake" \
  -DCMAKE_BUILD_TYPE=Release

# Сборка
cmake --build build --config Release
```
Если SQLite установлен иначе, убедитесь, что `find_package(SQLite3 REQUIRED)` находит пакет и предоставляет цель `SQLite::SQLite3`.

## Запуск

В каталоге сборки появятся:
- `tcp_chat_server` — сервер
- `tcp_chat_client` — клиент

1) Запустите сервер:
```bash
./tcp_chat_server
```
Сервер создаст базу `tcp_chat.db` (если её нет) и добавит `admin/admin123` при первом запуске.

2) Запустите клиент:
```bash
./tcp_chat_client
```

## Использование (клиент)

Главное меню:
- 1 — Вход
- 2 — Регистрация
- 3 — Выход

Меню пользователя:
- 1 — Приватный чат
- 2 — Общий чат
- 3 — История сообщений
- 4 — Список пользователей
- 5 — Выход (из аккаунта)

Команды в чате:
- `/exit` — вернуться в меню
- `/history` — показать историю (общую или приватную)
- `/users` — список пользователей (в общем чате)
- `/help` — показать команды
- `/quit` — выйти из приложения (в общем чате)

## Сеть

- Сервер слушает `0.0.0.0:12345`
- Клиент по умолчанию подключается к `127.0.0.1:12345` (см. `src/chat_client_tcp/client_main.cpp`)
- В клиенте настроен таймаут чтения для корректного выхода из аккаунта

## Частые проблемы

- CMake не находит SQLite3:
  - macOS: `brew install sqlite`
  - Linux: `sudo apt install libsqlite3-dev`
  - Windows: vcpkg (`vcpkg install sqlite3`) и `-DCMAKE_TOOLCHAIN_FILE`

- Клиент не может войти/зарегистрироваться:
  - Убедитесь, что сервер запущен и доступен по IP/порту
  - Проверьте брандмауэр/антивирус

## Разработка

- Простая многопоточность на сервере (`std::thread`, `recv`/`send`)
- Синхронизация (`std::mutex`) для списков клиентов
- Хранение пользователей/сообщений/логов в SQLite

## Лицензия

MIT
