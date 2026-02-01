#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <thread>
#include <vector>
#include <csignal>      
#include <atomic>
#include <sstream> 
#include <fstream> 
#include <set>     

#include "http.h"
#include "colors.h"
#include "banner.h"
#include "blacklist.h"
#include "logger.h"     

//  GLOBAL STATS 
std::atomic<int> total_requests(0);
std::atomic<int> blocked_requests(0);
std::atomic<int> honeypot_hits(0);
std::atomic<int> banned_ips_count(0);

//  CONFIG 
struct Config {
    int port = 8080;
    bool honeypot_mode = false;
    std::string admin_key = "secret123"; 
    std::string whitelist_file = "whitelist.txt"; 
};
Config global_config; 
Blacklist blacklist; 
Logger logger;          

//  VIP LIST (Whitelist) 
std::set<std::string> vip_list;

// Forward Declarations
std::pair<bool, std::string> check_security(const HttpRequest& req);
std::string inspect_response_body(const std::string& response);

//  WHITELIST LOADER 
void load_whitelist() {
    std::ifstream file(global_config.whitelist_file);
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            vip_list.insert(line);
        }
        file.close();
        std::cout << "  [INIT] Loaded " << vip_list.size() << " VIP IPs from whitelist." << std::endl;
    } else {
        std::cout << "  [!] Warning: No whitelist.txt found." << std::endl;
    }
}

//  EXIT HANDLER 
void signal_handler(int signum) {
    std::cout << "\n\n" << RESET;
    std::cout << "=============================================" << std::endl;
    std::cout << "           SENTILIGHT SESSION SUMMARY        " << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "  [+] Total Traffic Checked : " << CYAN << total_requests << RESET << std::endl;
    std::cout << "  [+] Threats Blocked       : " << RED << blocked_requests << RESET << std::endl;
    if (global_config.honeypot_mode) {
        std::cout << "  [+] Honeypot Triggers     : " << MAGENTA << honeypot_hits << RESET << std::endl;
    }
    std::cout << "  [+] IPs Banned            : " << RED << banned_ips_count << RESET << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "  Log saved to: sentilight.log" << std::endl;
    std::cout << "  Shutting down..." << std::endl;
    exit(signum);
}

//  HONEYPOT GENERATOR 
std::string generate_honeypot_response() {
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json\r\n"
           "Server: Apache/2.4.41 (Ubuntu)\r\n\r\n"
           "{\"error\": \"Uncaught Exception: MySQL syntax error near 'UNION' at line 1\", \"db_status\": \"unstable\", \"trace\": \"0x84F2A\"}";
}

//  STATS GENERATOR 
std::string generate_stats_response() {
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Content-Type: application/json\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n\r\n"; 
    
    ss << "{\n";
    ss << "  \"status\": \"online\",\n";
    ss << "  \"total_requests\": " << total_requests << ",\n";
    ss << "  \"blocked_attacks\": " << blocked_requests << ",\n";
    ss << "  \"honeypot_hits\": " << honeypot_hits << ",\n";
    ss << "  \"banned_ips\": " << banned_ips_count << "\n";
    ss << "}";
    
    return ss.str();
}

//  PROXY BRIDGE 
std::string forward_to_backend(std::string client_request) {
    int backend_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_sock < 0) return "HTTP/1.1 502 Bad Gateway\r\n\r\nSocket Error";

    sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(9000); 
    backend_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(backend_sock, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
        close(backend_sock);
        return "HTTP/1.1 502 Bad Gateway\r\n\r\nBackend Down!";
    }

    send(backend_sock, client_request.c_str(), client_request.length(), 0);

    std::string full_response = "";
    char buffer[4096] = {0};
    int bytes = 0;
    while ((bytes = read(backend_sock, buffer, 4096)) > 0) {
        full_response.append(buffer, bytes);
    }
    close(backend_sock);

    std::string sanitized_response = inspect_response_body(full_response);
    
    if (sanitized_response != full_response) {
        std::cout << MAGENTA << "  [DLP] Sensitive data leak blocked!" << RESET << std::endl;
        logger.log("DLP", "SERVER", "Blocked outbound sensitive data leak.");
    }

    return sanitized_response;
}

//  CLIENT HANDLER 
void handle_client(int client_socket, std::string client_ip) {
    total_requests++; 

    
    // 0. THE VIP CHECK (Bypass Everything)
    if (vip_list.count(client_ip)) {
        char buffer[4096] = {0};
        int bytes_read = read(client_socket, buffer, 4096);
        if (bytes_read > 0) {
            std::string raw_data(buffer);
            HttpRequest req = parse_request(raw_data);
            
            std::cout << YELLOW << "[VIP] " << RESET << "[" << client_ip << "] Bypassed Security Checks: " << req.method << " " << req.path << std::endl;
            
            std::string backend_response = forward_to_backend(raw_data);
            send(client_socket, backend_response.c_str(), backend_response.length(), 0);
        }
        close(client_socket);
        return; 
    }

    // 1. CHECK BLACKLIST
    if (blacklist.is_banned(client_ip)) {
        blocked_requests++; 
        std::cout << RED << "[BLACKLIST] Connection rejected from " << client_ip << RESET << std::endl;
        logger.log("REJECTED", client_ip, "Connection dropped (IP is banned)");
        close(client_socket);
        return; 
    }

    // 2. CHECK RATE LIMIT
    bool is_flooding = blacklist.check_rate_limit(client_ip);
    if (is_flooding) {
        banned_ips_count++;
        std::cout << RED << "[DoS DETECTED] High traffic from " << client_ip << " - BANNING NOW!" << RESET << std::endl;
        logger.log("BANNED", client_ip, "DoS Attack Detected. Banned for 5 min.");
        close(client_socket);
        return;
    }

    char buffer[4096] = {0};
    int bytes_read = read(client_socket, buffer, 4096);

    if (bytes_read > 0) {
        std::string raw_data(buffer);
        HttpRequest req = parse_request(raw_data);

        // 3. ADMIN STATS CHECK
        if (req.path == "/sentilight-stats") {
             if (req.headers.count("x-admin-key") && req.headers["x-admin-key"] == global_config.admin_key) {
                 std::cout << GREEN << "  [ADMIN] Stats accessed by " << client_ip << RESET << std::endl;
                 std::string stats_json = generate_stats_response();
                 send(client_socket, stats_json.c_str(), stats_json.length(), 0);
             } else {
                 std::cout << RED << "  [ADMIN] Unauthorized stats attempt by " << client_ip << RESET << std::endl;
                 std::string resp = "HTTP/1.1 403 Forbidden\r\n\r\nAccess Denied.";
                 send(client_socket, resp.c_str(), resp.length(), 0);
             }
             close(client_socket);
             return; 
        }

        std::cout << CYAN << "[THREAD " << std::this_thread::get_id() << "] " << RESET 
                  << "[" << client_ip << "] " << req.method << " " << req.path << std::endl;

        std::pair<bool, std::string> verdict = check_security(req);
        
        if (verdict.first) {
            //  ATTACK DETECTED 
            blocked_requests++;

            bool now_banned = blacklist.add_strike(client_ip);
            int current_strikes = blacklist.get_strikes(client_ip);

            if (now_banned) {
                 banned_ips_count++; 
                 std::cout << RED << "  [!!!] MAX STRIKES REACHED: Adding " << client_ip << " to Blacklist! [!!!]" << RESET << std::endl;
                 logger.log("BANNED", client_ip, "Max strikes reached.");
            }

            if (global_config.honeypot_mode) {
                honeypot_hits++;
                std::cout << MAGENTA << "  [***] HONEYPOT TRIGGERED (Strike " << current_strikes << "/3) [***]" << RESET << std::endl;
                logger.log("HONEYPOT", client_ip, "Triggered honeypot payload: " + req.path);
                
                std::string fake_resp = generate_honeypot_response();
                send(client_socket, fake_resp.c_str(), fake_resp.length(), 0);
            } else {
                std::cout << RED << "  [!!!] BLOCKED (Strike " << current_strikes << "/3): " << verdict.second << " [!!!]" << RESET << std::endl;
                logger.log("BLOCKED", client_ip, "Attack detected: " + verdict.second + " Payload: " + req.path);

                std::string response = "HTTP/1.1 403 Forbidden\r\n\r\n[SENTILIGHT] Access Denied.";
                send(client_socket, response.c_str(), response.length(), 0);
            }

        } else {
            //  SAFE TRAFFIC 
            std::cout << GREEN << "  [OK] Forwarding..." << RESET << std::endl;
            std::string backend_response = forward_to_backend(raw_data);
            send(client_socket, backend_response.c_str(), backend_response.length(), 0);
        }
    }
    close(client_socket);
}

//  MAIN 
int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);

    // Arg Parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            show_banner();
            show_help();
            return 0;
        } else if (arg == "--honey") {
            global_config.honeypot_mode = true;
        } else if (arg == "--port") {
            if (i + 1 < argc) {
                try { global_config.port = std::stoi(argv[++i]); } 
                catch (...) { return 1; }
            } else { return 1; }
        } 
    }

    show_banner();
    
    // LOAD WHITELIST
    load_whitelist();

    show_loading();

    
    std::cout << "---------------------------------------------" << std::endl;
    std::cout << " MODE      : ";
    if (global_config.honeypot_mode) {
        std::cout << MAGENTA << "HONEYPOT (Deception Active)" << RESET;
    } else {
        std::cout << RED << "STRICT (Blocking Active)" << RESET;
    }
    std::cout << std::endl;
    
    std::cout << " PORT      : " << GREEN << global_config.port << RESET << std::endl;
    std::cout << " MONITOR   : " << GREEN << "/sentilight-stats" << RESET << std::endl;
    std::cout << " WHITELIST : " << GREEN << vip_list.size() << " IPs loaded" << RESET << std::endl;
    std::cout << "---------------------------------------------\n" << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(global_config.port);
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << RED << "[FATAL] Port busy!" << RESET << std::endl;
        return 1;
    }
    listen(server_fd, 10);

    std::cout << "[*] Sentilight is listening for traffic... \n" << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (new_socket < 0) continue;

        char* client_ip_c = inet_ntoa(client_addr.sin_addr);
        std::string client_ip = (client_ip_c) ? client_ip_c : "UNKNOWN";

        std::thread client_thread(handle_client, new_socket, client_ip);
        client_thread.detach(); 
    }
    return 0;
}