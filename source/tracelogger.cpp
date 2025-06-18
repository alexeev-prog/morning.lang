#include <iostream>
#include <string>

#include "tracelogger.hpp"

#include "_default.hpp"

std::string TraceLogger::Indent;

TraceLogger::TraceLogger(const char* filename, const char* funcname, int linenumber)
    : m_FILENAME(filename)
    , m_FUNCNAME(funcname) {
    std::cout << GREY_COLOR << "::Trace::  " << Indent << "Entering " << m_FUNCNAME << "() - (" << m_FILENAME
              << ":" << linenumber << ")" << RESET_STYLE << '\n';
    if (Indent.empty()) {
        Indent.append(START_INDENT_SYMBOL);
    } else {
        Indent.append(INDENT_SYMBOL);
    }
}

TraceLogger::~TraceLogger() {
    Indent.resize(Indent.length() - INDENT_LENGTH);
    std::cout << GREY_COLOR << "::Trace::  " << Indent << "Leaving " << m_FUNCNAME << "() - (" << m_FILENAME
              << ")" << RESET_STYLE << '\n';
}
