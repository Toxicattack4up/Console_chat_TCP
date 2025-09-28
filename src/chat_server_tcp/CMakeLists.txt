find_package(SQLite3 REQUIRED)

add_library(chat_server
    Server.cpp
    Database.cpp
)

target_include_directories(chat_server PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(chat_server PUBLIC SQLite::SQLite3)