# SQLite3 imported target is ensured by the root CMakeLists.txt

add_library(chat_server
    Server.cpp
    Database.cpp
)

target_include_directories(chat_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Prefer pkg-config vars from parent if present
if(SODIUM_INCLUDE_DIRS)
    target_include_directories(chat_server PUBLIC ${SODIUM_INCLUDE_DIRS})
endif()
if(SODIUM_LIBRARIES)
    target_link_libraries(chat_server PUBLIC SQLite3::SQLite3 ${SODIUM_LIBRARIES})
else()
    target_link_libraries(chat_server PUBLIC SQLite3::SQLite3)
endif()
