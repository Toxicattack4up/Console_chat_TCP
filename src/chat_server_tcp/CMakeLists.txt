add_library(chat_server
    Server.cpp
    Database.cpp
)

target_include_directories(chat_server PUBLIC ${CMAKE_SOURCE_DIR}/include)