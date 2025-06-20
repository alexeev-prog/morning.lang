#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <iomanip>

#include "_default.hpp"

class Logger {
public:
    enum class Level {
        NOTE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    template <typename... Args>
    static void log(Level level, const char* format, Args... args) {
        std::string formatted = format_message(format, args...);
        print_log(level, formatted);

        if (level == Level::CRITICAL) {
            print_traceback();
            std::exit(EXIT_FAILURE);
        }
    }

    static void push_expression(const std::string& context, const std::string& expr) {
        expression_stack_.emplace_back(context, expr);
        if (expression_stack_.size() > MAX_STACK_SIZE) {
            expression_stack_.erase(expression_stack_.begin());
        }
    }

    static void print_traceback() {
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

private:
    static constexpr size_t MAX_STACK_SIZE = 100;
    static constexpr size_t TRACEBACK_LIMIT = 5;
    static thread_local std::vector<std::pair<std::string, std::string>> expression_stack_;

    template <typename... Args>
    static auto format_message(const char* format, Args... args) -> std::string {
        int size = std::snprintf(nullptr, 0, format, args...);
        if (size < 0) return "";

        std::vector<char> buf(size + 1);
        std::snprintf(buf.data(), buf.size(), format, args...);
        return std::string(buf.data());
    }

    static void print_log(Level level, const std::string& message) {
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
};

thread_local std::vector<std::pair<std::string, std::string>> Logger::expression_stack_;

#define LOG_NOTE(...)    Logger::log(Logger::Level::NOTE, __VA_ARGS__)
#define LOG_DEBUG(...)   Logger::log(Logger::Level::DEBUG, __VA_ARGS__)
#define LOG_INFO(...)    Logger::log(Logger::Level::INFO, __VA_ARGS__)
#define LOG_WARN(...)    Logger::log(Logger::Level::WARNING, __VA_ARGS__)
#define LOG_ERROR(...)   Logger::log(Logger::Level::ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) Logger::log(Logger::Level::CRITICAL, __VA_ARGS__)

#define PUSH_EXPR_STACK(ctx, expr) Logger::push_expression(ctx, expr)
