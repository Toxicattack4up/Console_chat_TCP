cmake_minimum_required(VERSION 3.10)
project(TCP_Chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Найдём SQLite3 (для Windows рекомендуется установить через пакетный менеджер, напр. vcpkg)
find_package(SQLite3 REQUIRED)

# Общие include
include_directories(include)

# Общая библиотека
add_library(common src/common.cpp)
target_include_directories(common PUBLIC include)

# Подпроекты (библиотеки)
add_subdirectory(src/chat_client_tcp)
add_subdirectory(src/chat_server_tcp)

# ==== Клиент ====
add_executable(tcp_chat_client
    src/Menu.cpp
    src/common.cpp
    src/chat_client_tcp/Client.cpp
    src/chat_client_tcp/client_main.cpp
)

target_link_libraries(tcp_chat_client PRIVATE chat_client)

# ==== Сервер ====
add_executable(tcp_chat_server
    src/chat_server_tcp/server_main.cpp
    src/chat_server_tcp/Database.cpp
    src/chat_server_tcp/Server.cpp
    src/common.cpp
)

target_link_libraries(tcp_chat_server PRIVATE chat_server SQLite::SQLite3)

# Сетевые библиотеки под Windows
if(WIN32)
    target_link_libraries(tcp_chat_client PRIVATE ws2_32)
    target_link_libraries(tcp_chat_server PRIVATE ws2_32)
endif()

# ==== База данных (копирование после сборки клиента) ====
# Копирование базы данных (если присутствует в проекте)
if(EXISTS ${CMAKE_SOURCE_DIR}/database/tcp_chat.db)
    add_custom_command(TARGET tcp_chat_server POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/database/tcp_chat.db
        ${CMAKE_BINARY_DIR}/tcp_chat.db)
    add_custom_command(TARGET tcp_chat_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/database/tcp_chat.db
        ${CMAKE_BINARY_DIR}/tcp_chat.db)
endif()