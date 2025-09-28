#pragma once
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>

extern std::mutex io_mutex;

void logInfo(const std::string &msg);
void logError(const std::string &msg);
void logDebug(const std::string &msg);
