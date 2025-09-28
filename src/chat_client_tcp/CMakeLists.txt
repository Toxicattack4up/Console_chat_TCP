add_library(chat_client
    Client.cpp
)

target_include_directories(chat_client PUBLIC ${CMAKE_SOURCE_DIR}/include)