#include "logger.hpp"
#include <sstream>
#include <iomanip>

thread_local std::vector<std::pair<std::string, std::string>> Logger::expression_stack_;

void Logger::push_expression(const std::string& context, const std::string& expr) {
    expression_stack_.emplace_back(context, expr);
    if (expression_stack_.size() > MAX_STACK_SIZE) {
        expression_stack_.erase(expression_stack_.begin());
    }
}

void Logger::print_traceback() {
    if (expression_stack_.empty()) return;

    std::fprintf(stderr, "%sExpressions traceback:%s\n", BOLD, RESET_STYLE);

    size_t start = expression_stack_.size() > TRACEBACK_LIMIT ?
        expression_stack_.size() - TRACEBACK_LIMIT : 0;

    for (size_t i = start; i < expression_stack_.size(); ++i) {
        const auto& [ctx, expr] = expression_stack_[i];
        std::fprintf(stderr, "    %s%-8s%s %s\n",
                     CYAN_COLOR, ctx.c_str(), RESET_STYLE, expr.c_str());
    }
}

void Logger::print_log(Level level, const std::string& message) {
    const char* level_str = "";
    const char* color = "";
    FILE* stream = stdout;

    switch (level) {
        case Level::NOTE:
            level_str = "NOTE";
            color = GREEN_COLOR;
            break;
        case Level::DEBUG:
            level_str = "DEBUG";
            color = CYAN_COLOR;
            break;
        case Level::INFO:
            level_str = "INFO";
            color = BLUE_COLOR;
            stream = stdout;
            break;
        case Level::WARNING:
            level_str = "WARNING";
            color = YELLOW_COLOR;
            stream = stderr;
            break;
        case Level::ERROR:
            level_str = "ERROR";
            color = RED_COLOR;
            stream = stderr;
            break;
        case Level::CRITICAL:
            level_str = "CRITICAL";
            color = PURPLE_COLOR;
            stream = stderr;
            break;
    }

    std::fprintf(stream, "%s[MORNINGLLVM :: %s%s%-8s%s]%s %s\n",
                 BOLD, BOLD, color, level_str, RESET_STYLE, RESET_STYLE,
                 message.c_str());
    std::fflush(stream);
}
