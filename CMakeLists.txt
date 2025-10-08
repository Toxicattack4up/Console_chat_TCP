cmake_minimum_required(VERSION 3.15)  # Повышаем версию для лучшей поддержки
project(TCP_Chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Функция для автоматической установки SQLite3
function(install_sqlite3_dependencies)
    if(WIN32)
        message(STATUS "Проверка vcpkg для установки SQLite3...")
        
        # Проверяем наличие vcpkg
        find_program(VCPKG_EXECUTABLE vcpkg PATHS "C:/vcpkg" "$ENV{HOME}/vcpkg" "C:/tools/vcpkg")
        if(NOT VCPKG_EXECUTABLE)
            message(WARNING "vcpkg не найден. Установите vcpkg для автоматической установки зависимостей")
            message(STATUS "Ручная установка: git clone https://github.com/Microsoft/vcpkg.git && cd vcpkg && .\\bootstrap-vcpkg.bat")
            return()
        endif()
        
        # Устанавливаем SQLite3 через vcpkg
        execute_process(
            COMMAND ${VCPKG_EXECUTABLE} install sqlite3:x64-windows
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE result
        )
        
        if(result EQUAL 0)
            message(STATUS "SQLite3 успешно установлен через vcpkg")
            set(CMAKE_TOOLCHAIN_FILE "${VCPKG_EXECUTABLE}/../scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain")
        else()
            message(WARNING "Не удалось установить SQLite3 через vcpkg")
        endif()
        
    elseif(APPLE)
        message(STATUS "Проверка Homebrew для установки SQLite3...")
        
        # Проверяем наличие Homebrew
        find_program(BREW_EXECUTABLE brew)
        if(NOT BREW_EXECUTABLE)
            message(WARNING "Homebrew не найден. Установите Homebrew для автоматической установки зависимостей")
            message(STATUS "Ручная установка: /bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"")
            return()
        endif()
        
        # Устанавливаем SQLite3 через Homebrew
        execute_process(
            COMMAND ${BREW_EXECUTABLE} install sqlite
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE result
        )
        
        if(result EQUAL 0)
            message(STATUS "SQLite3 успешно установлен через Homebrew")
        else()
            message(WARNING "Не удалось установить SQLite3 через Homebrew")
        endif()
        
    elseif(UNIX AND NOT APPLE)
        message(STATUS "Проверка системного менеджера пакетов для Linux...")
        
        # Для Linux можно добавить логику для apt, yum, etc.
        find_program(APT_EXECUTABLE apt)
        find_program(YUM_EXECUTABLE yum)
        
        if(APT_EXECUTABLE)
            execute_process(
                COMMAND sudo ${APT_EXECUTABLE} install -y libsqlite3-dev
                RESULT_VARIABLE result
            )
        elseif(YUM_EXECUTABLE)
            execute_process(
                COMMAND sudo ${YUM_EXECUTABLE} install -y sqlite-devel
                RESULT_VARIABLE result
            )
        else()
            message(WARNING "Не найден поддерживаемый менеджер пакетов для Linux")
            return()
        endif()
        
        if(result EQUAL 0)
            message(STATUS "SQLite3 успешно установлен")
        else()
            message(WARNING "Не удалось установить SQLite3")
        endif()
    endif()
endfunction()

# Пытаемся найти SQLite3
find_package(SQLite3 QUIET)

# Если не найден - пытаемся установить
if(NOT SQLite3_FOUND)
    message(STATUS "SQLite3 не найден, пытаемся установить автоматически...")
    install_sqlite3_dependencies()
    
    # Пытаемся найти снова после установки
    find_package(SQLite3 REQUIRED)
endif()

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