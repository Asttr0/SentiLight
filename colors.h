#ifndef COLORS_H
#define COLORS_H

#include <string>

// ANSI Escape Codes for Terminal Colors
// Added 'inline' to prevent linker errors
inline const std::string RED     = "\033[1;31m";
inline const std::string GREEN   = "\033[1;32m";
inline const std::string YELLOW  = "\033[1;33m";
inline const std::string BLUE    = "\033[1;34m";
inline const std::string MAGENTA = "\033[1;35m";
inline const std::string CYAN    = "\033[1;36m";
inline const std::string RESET   = "\033[0m";

#endif