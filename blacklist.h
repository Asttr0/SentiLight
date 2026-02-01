#ifndef BLACKLIST_H
#define BLACKLIST_H

#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <mutex>
#include <deque> 

class Blacklist {
private:
    std::map<std::string, int> strikes;
    std::map<std::string, std::chrono::steady_clock::time_point> ban_list;
    
    // Tracks request times: "127.0.0.1" -> [Time1, Time2...]
    std::map<std::string, std::deque<std::chrono::steady_clock::time_point>> request_history;
    
    std::mutex list_mutex;

    const int MAX_STRIKES = 3;
    const int BAN_DURATION_SECONDS = 300; 
    
    const size_t RATE_LIMIT_MAX_REQ = 10;    
    const int RATE_LIMIT_WINDOW = 1;      

public:
    // 1. CHECK IF BANNED
    bool is_banned(const std::string& ip) {
        std::lock_guard<std::mutex> lock(list_mutex);
        if (ban_list.count(ip)) {
            auto now = std::chrono::steady_clock::now();
            if (now < ban_list[ip]) {
                return true; 
            } else {
                ban_list.erase(ip);
                strikes[ip] = 0;
                request_history.erase(ip); 
                return false;
            }
        }
        return false;
    }

    // 2. CHECK RATE LIMIT
    bool check_rate_limit(const std::string& ip) {
        std::lock_guard<std::mutex> lock(list_mutex);

        auto now = std::chrono::steady_clock::now();
        auto& history = request_history[ip];

        history.push_back(now);

        // Remove old requests
        while (!history.empty()) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - history.front()).count();
            if (duration > RATE_LIMIT_WINDOW) {
                history.pop_front();
            } else {
                break; 
            }
        }

        if (history.size() > RATE_LIMIT_MAX_REQ) {
            auto ban_end = std::chrono::steady_clock::now() + std::chrono::seconds(BAN_DURATION_SECONDS);
            ban_list[ip] = ban_end;
            return true; 
        }

        return false;
    }

    // 3. ADD STRIKE
    bool add_strike(const std::string& ip) {
        std::lock_guard<std::mutex> lock(list_mutex);
        strikes[ip]++;
        if (strikes[ip] >= MAX_STRIKES) {
            auto ban_end = std::chrono::steady_clock::now() + std::chrono::seconds(BAN_DURATION_SECONDS);
            ban_list[ip] = ban_end;
            return true; 
        }
        return false;
    }

    int get_strikes(const std::string& ip) {
        std::lock_guard<std::mutex> lock(list_mutex);
        return strikes[ip];
    }
};

#endif