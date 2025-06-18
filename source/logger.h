#pragma once

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "_default.hpp"

class ErrorLogMessage : public std::basic_ostringstream<char> {
  public:
    ~ErrorLogMessage() override {
        std::cerr << RED_COLOR << BOLD << "Fatal error: " << str().c_str() << RESET_STYLE;
        exit(EXIT_FAILURE);
    }
};

#define DIE ErrorLogMessage()
