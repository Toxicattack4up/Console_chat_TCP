#pragma once
#include <mutex>
#include <iostream>

// console IO mutex
extern std::mutex io_mutex;

// Logging API: thread-safe functions that append to chat.log and echo to console
void logInfo(const std::string &msg);
void logError(const std::string &msg);
void logDebug(const std::string &msg);
