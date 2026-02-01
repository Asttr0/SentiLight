#include "http.h"
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <fstream>
#include <regex> 

//  CONFIGURATION 
const double ENTROPY_THRESHOLD = 4.8;
const std::string RULES_FILE = "rules.txt";

//  UTILS 
std::string url_decode(const std::string& input) {
    std::string decoded = "";
    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == '%' && i + 2 < input.length()) {
            std::string hex = input.substr(i + 1, 2);
            char ch = (char)strtol(hex.c_str(), nullptr, 16);
            decoded += ch;
            i += 2;
        } else if (input[i] == '+') {
            decoded += ' ';
        } else {
            decoded += input[i];
        }
    }
    return decoded;
}

std::string to_lower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

//  STRUCTURE FOR RULES 
struct WafRule {
    std::regex pattern;
    std::string original_text; 
};

//  RULE LOADER 
std::vector<WafRule> load_rules() {
    std::vector<WafRule> rules;
    std::ifstream file(RULES_FILE);
    std::string line;
    
    std::cout << "  [INIT] Loading Rules from " << RULES_FILE << "..." << std::endl;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (line[0] == '#') continue;

            try {
                std::regex re(line, std::regex::optimize | std::regex::icase);
                rules.push_back({re, line});
            } catch (const std::regex_error& e) {
                std::cerr << "  [!] Error compiling rule '" << line << "': " << e.what() << std::endl;
            }
        }
        file.close();
        std::cout << "  [INIT] Successfully loaded " << rules.size() << " advanced signatures." << std::endl;
    } else {
        std::cerr << "  [!] Warning: Could not open " << RULES_FILE << ". No signatures loaded!" << std::endl;
    }
    return rules;
}

//  ENTROPY ENGINE 
double calculate_entropy(const std::string& input) {
    if (input.empty()) return 0.0;
    std::map<char, int> frequencies;
    for (char c : input) frequencies[c]++;
    double entropy = 0.0;
    double len = (double)input.length(); 
    for (auto const& item : frequencies) {
        double p = item.second / len; 
        entropy -= p * log2(p);
    }
    return entropy;
}

//  DLP: THE CENSOR  
std::string inspect_response_body(const std::string& response) {
    std::vector<std::string> leaks = {
        "Fatal error:",         
        "SQL syntax",           
        "Index of /",           
        "root:x:0:0:",          
        "Internal Server Error" 
    };

    for (const auto& leak : leaks) {
        if (response.find(leak) != std::string::npos) {
            return "HTTP/1.1 500 Internal Server Error\r\n"
                   "Content-Type: text/plain\r\n"
                   "Content-Length: 45\r\n\r\n"
                   "[SENTILIGHT] Response Redacted: Leak Detected";
        }
    }
    return response;
}

//  MAIN SECURITY CHECK 
std::pair<bool, std::string> check_security(const HttpRequest& req) {
    
    // 1. PROTOCOL ENFORCEMENT
    if (req.method != "GET" && req.method != "POST") {
        return {true, "Method Not Allowed: " + req.method};
    }

    // 2. NORMALIZE & DECODE
    std::string full_payload = req.path + " " + req.body; 
    std::string decoded_payload = url_decode(full_payload);

    // 3. THE BOUNCER (User-Agent Blocking)
    if (req.headers.count("user-agent")) {
        std::string ua = to_lower(req.headers.at("user-agent"));
        std::vector<std::string> bad_bots = {
            "sqlmap", "nikto", "hydra", "burp", "metasploit", "nmap", "gobuster"
        };
        for (const auto& bot : bad_bots) {
            if (ua.find(bot) != std::string::npos) {
                return {true, "Blacklisted Tool Detected: " + bot};
            }
        }
    }

    // 4. ADVANCED REGEX DETECTION
    static std::vector<WafRule> signatures = load_rules();

    for (const auto& rule : signatures) {
        if (std::regex_search(decoded_payload, rule.pattern)) {
            return {true, "Signature Match: " + rule.original_text};
        }
    }

    // 5. ENTROPY DETECTION
    if (req.path.length() > 15) {
        double score = calculate_entropy(req.path);
        if (score > ENTROPY_THRESHOLD) {
            return {true, "High Entropy / Obfuscation (Score: " + std::to_string(score) + ")"};
        }
    }

    return {false, "Safe"};
}