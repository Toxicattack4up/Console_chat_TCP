#include "../include/common.h"

std::mutex io_mutex;


static std::mutex log_mutex;

static std::string currentTimestamp() {
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);
	std::tm tm = *std::localtime(&t);
	std::ostringstream ss;
	ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

static void writeLog(const std::string &level, const std::string &msg) {
	std::lock_guard<std::mutex> lock(log_mutex);
	std::ofstream ofs("chat.log", std::ios::app);
	if (!ofs) return;
	ofs << "[" << currentTimestamp() << "] [" << level << "] " << msg << std::endl;
	ofs.close();
}

void logInfo(const std::string &msg) {
	std::lock_guard<std::mutex> lock(io_mutex);
	writeLog("INFO", msg);
	std::cout << msg << std::endl;
}

void logError(const std::string &msg) {
	std::lock_guard<std::mutex> lock(io_mutex);
	writeLog("ERROR", msg);
	std::cerr << msg << std::endl;
}

void logDebug(const std::string &msg) {
	writeLog("DEBUG", msg);
}
