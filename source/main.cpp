#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include "morningllvm.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <optional>
#include <sstream>

#include "logger.hpp"
#include "input_parser.hpp"

namespace fs = std::filesystem;

namespace {
    /**
     * @brief Check if util is available (cross-platform)
     */
    auto is_util_available(const std::string& util) -> bool {
        #ifdef _WIN32
        std::string cmd = "where " + util + " >nul 2>nul";
        #else
        std::string cmd = "command -v " + util + " >/dev/null 2>&1";
        #endif
        return std::system(cmd.c_str()) == 0;
    }

    /**
     * @brief Safe command execution
     */
    auto execute_command(const std::string& cmd, bool quiet = true) -> int {
        if (quiet) {
            #ifdef _WIN32
            return std::system((cmd + " >nul 2>nul").c_str());
            #else
            return std::system((cmd + " >/dev/null 2>&1").c_str());
            #endif
        } else {
            return std::system(cmd.c_str());
        }
    }

    /**
     * @brief Generate safe quoted path
     */
    auto safe_path(const std::string& path) -> std::string {
        if (path.empty()) return "\"\"";
        if (path.find(' ') != std::string::npos) {
            return "\"" + path + "\"";
        }
        return path;
    }

    /**
     * @brief Compile generated IR to binary
     */
    auto compile_ir(const std::string& output_base) -> bool {
        const std::string ll_file = output_base + ".ll";
        const std::string opt_ll_file = output_base + "-opt.ll";
        const std::string bin_file = output_base;

        if (!fs::exists(ll_file)) {
            LOG_ERROR("IR code not found");
            return false;
        }

        std::string opt_cmd = "opt " + safe_path(ll_file) +
                              " -O3 -S -o " + safe_path(opt_ll_file);

        LOG_INFO("Optimizing code...");

        if (execute_command(opt_cmd) != 0) {
            LOG_ERROR("Code optimization failed");
            std::cout << "Command: " << opt_cmd << "\n";
            execute_command(opt_cmd, false);
            return false;
        }

        if (!fs::exists(opt_ll_file) || fs::file_size(opt_ll_file) == 0) {
            LOG_ERROR("Optimized IR code not created");
            return false;
        }

        std::string clang_cmd = "clang++ -O3 " + safe_path(opt_ll_file) +
                                " -o " + safe_path(bin_file);

        LOG_INFO("Compiling optimized code...");

        if (execute_command(clang_cmd) != 0) {
            LOG_ERROR("Binary compilation failed");
            std::cout << "Command: " << clang_cmd << "\n";
            execute_command(clang_cmd, false);
            return false;
        }

        if (!fs::exists(bin_file) || fs::file_size(bin_file) == 0) {
            LOG_ERROR("Binary file \"%s\" not created", bin_file.c_str());
            return false;
        }

        return true;
    }

    /**
     * @brief Safe cleanup of temporary files
     */
    void cleanup_temp_files(const std::string& output_base) {
        auto safe_remove = [](const std::string& path) {
            try {
                if (fs::exists(path)) {
                    fs::remove(path);
                    LOG_DEBUG("Removed temp file: %s", path.c_str());
                }
            } catch (...) {
                LOG_WARN("Could not remove file \"%s\"", path.c_str());
            }
        };

        safe_remove(output_base + ".ll");
        safe_remove(output_base + "-opt.ll");
    }

    /**
     * @brief Check if all required utils are available
     */
    auto check_utils_available() -> bool {
        const std::vector<std::string> REQUIRED_PROGS = {"opt", "clang++"};

        for (const auto& util : REQUIRED_PROGS) {
            if (!is_util_available(util)) {
                LOG_ERROR("Required utility \"%s\" not found. Please install it.", util.c_str());
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Check if output name is valid
     */
    auto is_valid_output_name(const std::string& name) -> bool {
        if (name.empty()) return false;

        const std::string FORBIDDEN_CHARS = "/\\:*?\"<>|";
        return std::none_of(name.begin(), name.end(), [&](char c) {
            return FORBIDDEN_CHARS.find(c) != std::string::npos;
        });
    }
}

/**
 * @brief Entry point
 */
auto main(int argc, char **argv) -> int {
    const std::string VERSION = "0.8.0";

    MorningLanguageLLVM morning_vm;
    std::string program;
    std::string output_base = "out";
    bool compile_raw_object_file = false;

    // Initialize parser with program info
    InputParser parser(
        fs::path(argv[0]).filename().string(),
        "MorningLLVM - Compiler for the Morning programming language"
    );

    // Register command line options
    parser.add_option({"-v", "--version", "Get version", false, ""});
    parser.add_option({"-h", "--help", "Print this help message", false, ""});
    parser.add_option({"-e", "--expression", "Expression to parse", true, "<expr>"});
    parser.add_option({"-f", "--file", "File to parse", true, "<file>"});
    parser.add_option({"-o", "--output", "Output binary name", true, "<name>"});
    parser.add_option({"-k", "--keep", "Keep temporary files", false, ""});
    parser.add_option({"-cof", "--compile-object-file", "Compile raw object file", false, ""});

    // Parse command line
    if (!parser.parse(argc, argv)) {
        for (const auto& error : parser.get_errors()) {
            LOG_ERROR("%s", error.c_str());
        }
        std::cerr << parser.generate_help() << "\n";
        return 1;
    }

    if (parser.has_option("-v")) {
        LOG_INFO("Version: %s", VERSION.c_str());
        return 0;
    }

    // Handle help option
    if (parser.has_option("-h") || parser.has_option("--help")) {
        std::cout << parser.generate_help() << "\n";
        return 0;
    }

    if (parser.has_option("-cof") || parser.has_option("--compile-object-file")) {
        compile_raw_object_file = true;
    }

    // Handle output option
    if (auto output = parser.get_argument("-o")) {
        output_base = *output;
    } else if (auto output = parser.get_argument("--output")) {
        output_base = *output;
    }

    if (!is_valid_output_name(output_base)) {
        LOG_ERROR("Invalid output name: %s", output_base.c_str());
        return 1;
    }

    // Handle input source
    if (auto filename = parser.get_argument("-f")) {
        if (!fs::exists(*filename)) {
            LOG_ERROR("File \"%s\" not found", filename->c_str());
            return 1;
        }

        std::ifstream program_file(*filename);
        if (!program_file.is_open()) {
            LOG_ERROR("Cannot open file \"%s\"", filename->c_str());
            return 1;
        }

        std::stringstream buffer;
        buffer << program_file.rdbuf();
        program = buffer.str();

        if (program.empty()) {
            LOG_ERROR("File \"%s\" is empty", filename->c_str());
            return 1;
        }
    } else if (auto expr = parser.get_argument("-e")) {
        program = *expr;

        if (program.empty()) {
            LOG_ERROR("Empty expression");
            return 1;
        }
    } else {
        LOG_ERROR("No input specified (use -e or -f)");
        std::cerr << parser.generate_help() << "\n";
        return 1;
    }

    // Check required utilities
    if (!check_utils_available()) {
        return 1;
    }

    // Execute compilation pipeline
    try {
        LOG_INFO("Executing program...\n");
        morning_vm.execute(program, output_base);
        std::cout << "\n";

        const std::string LL_FILE = output_base + ".ll";
        if (!fs::exists(LL_FILE) || fs::file_size(LL_FILE) == 0) {
            LOG_ERROR("IR generation failed, no output file");
            return 1;
        }

        if (!compile_ir(output_base)) {
            LOG_ERROR("Compilation failed, temporary files retained for debugging");
            return 1;
        }

        // Cleanup temporary files
        if (!parser.has_option("-k") && !parser.has_option("--keep")) {
            cleanup_temp_files(output_base);
        } else {
            LOG_INFO("Optimized IR code saved: %s", LL_FILE.c_str());
        }

        LOG_INFO("Successfully compiled to %s", output_base.c_str());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Fatal error");
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
