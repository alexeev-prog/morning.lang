#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>

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

    // Шаблонные методы остаются в заголовке
    template <typename... Args>
    static void log(Level level, const char* format, Args... args) {
        std::string formatted = format_message(format, args...);
        print_log(level, formatted);

        if (level == Level::CRITICAL) {
            print_traceback();
            std::exit(EXIT_FAILURE);
        }
    }

    static void push_expression(const std::string& context, const std::string& expr);
    static void print_traceback();

private:
    static constexpr size_t MAX_STACK_SIZE = 100;
    static constexpr size_t TRACEBACK_LIMIT = 5;
    static thread_local std::vector<std::pair<std::string, std::string>> expression_stack_;

    // Приватный шаблонный метод
    template <typename... Args>
    static std::string format_message(const char* format, Args... args) {
        int size = std::snprintf(nullptr, 0, format, args...);
        if (size < 0) return "";

        std::vector<char> buf(size + 1);
        std::snprintf(buf.data(), buf.size(), format, args...);
        return std::string(buf.data());
    }

    static void print_log(Level level, const std::string& message);
};

#define LOG_NOTE(...)    Logger::log(Logger::Level::NOTE, __VA_ARGS__)
#define LOG_DEBUG(...)   Logger::log(Logger::Level::DEBUG, __VA_ARGS__)
#define LOG_INFO(...)    Logger::log(Logger::Level::INFO, __VA_ARGS__)
#define LOG_WARN(...)    Logger::log(Logger::Level::WARNING, __VA_ARGS__)
#define LOG_ERROR(...)   Logger::log(Logger::Level::ERROR, __VA_ARGS__)
#define LOG_CRITICAL(...) Logger::log(Logger::Level::CRITICAL, __VA_ARGS__)

#define PUSH_EXPR_STACK(ctx, expr) Logger::push_expression(ctx, expr)
