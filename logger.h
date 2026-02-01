#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iomanip>

class Logger {
private:
    std::mutex log_mutex;
    const std::string filename = "sentilight.log";

    // Helper to get current time as string: "[2026-02-01 10:00:00]"
    std::string get_timestamp() {
        auto now = std::time(nullptr);
        auto tm = *std::localtime(&now);
        std::stringstream ss;
        ss << std::put_time(&tm, "[%Y-%m-%d %H:%M:%S]");
        return ss.str();
    }

public:
    // Write a message to the file
    void log(const std::string& type, const std::string& ip, const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::ofstream file;
        file.open(filename, std::ios::app); // Open in "Append" mode (don't overwrite)
        
        if (file.is_open()) {
            file << get_timestamp() << " [" << type << "] " 
                 << "[" << ip << "] " << message << std::endl;
            file.close();
        }
    }
};

#endif