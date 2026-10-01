cmake_minimum_required(VERSION 3.15)
project(TCP_Chat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

find_package(SQLite3)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND AND NOT TARGET SQLite3::SQLite3)
    pkg_check_modules(PC_SQLITE3 QUIET sqlite3)
endif()

if(NOT TARGET SQLite3::SQLite3)
    set(_sqlite_lib "${SQLite3_LIBRARIES}")
    set(_sqlite_inc "${SQLite3_INCLUDE_DIRS}")
    if(NOT _sqlite_lib AND SQLite3_LIBRARY)
        set(_sqlite_lib "${SQLite3_LIBRARY}")
    endif()
    if(NOT _sqlite_inc AND SQLite3_INCLUDE_DIR)
        set(_sqlite_inc "${SQLite3_INCLUDE_DIR}")
    endif()
    if(NOT _sqlite_lib AND PC_SQLITE3_LIBRARIES)
        set(_sqlite_lib "${PC_SQLITE3_LIBRARIES}")
        set(_sqlite_inc "${PC_SQLITE3_INCLUDE_DIRS}")
    endif()
    if(NOT _sqlite_lib)
        message(FATAL_ERROR "SQLite3 not found. Install libsqlite3-dev (Ubuntu) or sqlite (macOS).")
    endif()
    add_library(SQLite3::SQLite3 INTERFACE IMPORTED GLOBAL)
    set_target_properties(SQLite3::SQLite3 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_sqlite_inc}"
        INTERFACE_LINK_LIBRARIES "${_sqlite_lib}"
    )
endif()

if(PkgConfig_FOUND)
    pkg_check_modules(SODIUM QUIET libsodium)
endif()

if(NOT SODIUM_FOUND)
    find_path(SODIUM_INCLUDE_DIRS sodium.h)
    find_library(SODIUM_LIBRARIES sodium)
    if(SODIUM_INCLUDE_DIRS AND SODIUM_LIBRARIES)
        set(SODIUM_FOUND TRUE)
    endif()
endif()

if(NOT SODIUM_FOUND)
    message(FATAL_ERROR "libsodium not found. Install it (macOS: brew install libsodium, Ubuntu: sudo apt install libsodium-dev)")
endif()

include_directories(include)

add_library(common src/common.cpp)
target_include_directories(common PUBLIC include)

add_subdirectory(src/chat_client_tcp)
add_subdirectory(src/chat_server_tcp)

add_executable(tcp_chat_client
    src/Menu.cpp
    src/common.cpp
    src/chat_client_tcp/Client.cpp
    src/chat_client_tcp/client_main.cpp
)
target_link_libraries(tcp_chat_client PRIVATE chat_client)

add_executable(tcp_chat_server
    src/chat_server_tcp/server_main.cpp
    src/chat_server_tcp/Database.cpp
    src/chat_server_tcp/Server.cpp
    src/common.cpp
)
target_include_directories(tcp_chat_server PRIVATE ${SODIUM_INCLUDE_DIRS})
target_link_libraries(tcp_chat_server PRIVATE chat_server SQLite3::SQLite3 ${SODIUM_LIBRARIES})

if(WIN32)
    target_link_libraries(tcp_chat_client PRIVATE ws2_32)
    target_link_libraries(tcp_chat_server PRIVATE ws2_32)
endif()

# Tests
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(chat_db_tests
    tests/test_chat_db.cpp
    src/chat_server_tcp/Database.cpp
)
target_include_directories(chat_db_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/chat_server_tcp
    ${SODIUM_INCLUDE_DIRS}
)
target_link_libraries(chat_db_tests PRIVATE GTest::gtest_main SQLite3::SQLite3 ${SODIUM_LIBRARIES})
include(GoogleTest)
gtest_discover_tests(chat_db_tests)
