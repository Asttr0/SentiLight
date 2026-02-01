#ifndef BANNER_H
#define BANNER_H

#include <iostream>
#include <thread>
#include <chrono>
#include "colors.h"

inline void show_banner() {
    std::cout << "\033[2J\033[1;1H"; 

    std::cout << "\n";
    
    // PARTICLES (Top)
    std::cout << CYAN << "      .       +    " << YELLOW << " *" << CYAN << "       .      " << YELLOW << " + " << CYAN << "     .  " << RESET << CYAN << "      .       +    " << YELLOW << " *" << CYAN << std::endl;

    
    std::cout << CYAN << "   _________              __ . __ " << YELLOW << " .____    .__       .__     __   " << RESET << std::endl;
    std::cout << CYAN << "  /   _____/ ____   _____/  |_|__|" << YELLOW << " |    |   |__| ____ |  |___/  |_ " << RESET << std::endl;
    std::cout << CYAN << "  \\_____  \\_/ __ \\ /    \\   __\\  |" << YELLOW << " |    |   |  |/ ___ \\  |  \\   __\\" << RESET << std::endl;
    std::cout << CYAN << "  /        \\  ___/|   |  \\  | |  |" << YELLOW << " |    |___|  / /_/  >   |  \\  |  " << RESET << std::endl;
    std::cout << CYAN << " /_______  /\\___  >___|  /__| |__|" << YELLOW << " |_______ \\__\\___  /|___|  /__|  " << RESET << std::endl;
    std::cout << CYAN << "         \\/     \\/     \\/         " << YELLOW << "         \\/ /_____/      \\/      " << RESET << std::endl;

    // PARTICLES (Bottom)
    std::cout << CYAN << "    +      .        * " << YELLOW << "          |      " << CYAN << " .   " << YELLOW << " * " << RESET << CYAN << "    +      .        * " << std::endl;
    std::cout << "\n";

    // INFO
    std::cout << "      " << CYAN << "         :: " << RESET << "Your Pocket Modular WAF Engine " << CYAN <<   "::" << RESET << std::endl;
    std::cout << "\n";
    std::cout << "    [+] Author   : " << MAGENTA << "Asttr0" << RESET << std::endl;
    std::cout << "    [+] Version  : " << GREEN << "1.0.0" << RESET << std::endl;
    std::cout << "    [+] Status   : " << GREEN << "Online" << RESET << "\n" << std::endl;
}

inline void show_loading() {
    std::cout << "[*] " << CYAN << "Energizing Modules..." << RESET << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    std::cout << YELLOW << " [Ready]" << RESET << std::endl;

    std::cout << "[*] " << CYAN << "Calibrating Sensors..." << RESET << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    std::cout << YELLOW << " [Ready]" << RESET << std::endl;

    std::cout << "[*] " << CYAN << "Honeypot Grid..." << RESET << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    std::cout << GREEN << "       [ACTIVE]" << RESET << "\n" << std::endl;
}

inline void show_help() {
    std::cout << "Usage: ./sentilight [OPTIONS]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --port <p>    Set listening port (Default: 8080)" << std::endl;
    std::cout << "  --honey       Activate Honeypot Mode (Deceive attackers)" << std::endl;
    std::cout << "  --help        Show this message" << std::endl;
}

#endif