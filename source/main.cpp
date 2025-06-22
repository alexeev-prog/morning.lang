#include <boost/range/algorithm/find.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include "morningllvm.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <algorithm>

#include "logger.hpp"
#include "linter.hpp"

namespace fs = std::filesystem;

/**
 * @brief Anaonymous namespace
 *
 **/
namespace {
    /**
     * @brief Print help info
     *
     **/
    void print_help() {
        std::cout << "\nUsage: morningllvm [options]\n\n"
                  << "Options:\n"
                  << "    -e, --expression  Expression to parse\n"
                  << "    -f, --file        File to parse\n"
                  << "    --lint            File to lint\n"
                  << "    -o, --output      Output binary name\n\n";
    }

    /**
     * @brief Check is util is available (crossplatform)
     *
     * @param util util name
     * @return true
     * @return false
     **/
    auto is_util_available(const std::string& util) -> bool {
        #ifdef _WIN32
        std::string cmd = "where " + util + " >nul 2>nul";
        #else
        std::string cmd = "command -v " + util + " >/dev/null 2>&1";
        #endif
        return system(cmd.c_str()) == 0;
    }

    /**
     * @brief Safe command execution
     *
     * @param cmd command
     * @return int
     **/
    auto execute_command(const std::string& cmd, bool quiet = true) -> int {
        if (quiet) {
            #ifdef _WIN32
            return system((cmd + " >nul 2>nul").c_str());
            #else
            return system((cmd + " >/dev/null 2>&1").c_str());
            #endif
        } else {
            #ifdef _WIN32
            return system(cmd.c_str());
            #else
            return system(cmd.c_str());
            #endif
        }
    }

    /**
     * @brief Generate safe path
     *
     * @param path raw path
     * @return std::string
     **/
    auto safe_path(const std::string& path) -> std::string {
        if (path.empty()) return "\"\"";
        if (path.find(' ') != std::string::npos) {
            return "\"" + path + "\"";
        }
        return path;
    }

    /**
     * @brief Compile generated IR to binary
     *
     * @param output_base output base filename
     * @return true
     * @return false
     **/
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

        LOG_INFO("Try to optimize code...");

        if (execute_command(opt_cmd) != 0) {
            LOG_ERROR("Optimize code failed");
            std::cout << "COMMAND " << opt_cmd << " LOG ERROR:";
            execute_command(opt_cmd);
            return false;
        }

        if (!fs::exists(opt_ll_file) || fs::file_size(opt_ll_file) == 0) {
            LOG_ERROR("Optimized IR code not created");
            return false;
        }

        std::string clang_cmd = "clang++ -O3 " + safe_path(opt_ll_file) +
                                " -o " + safe_path(bin_file);

        LOG_INFO("Try to compile optimized code...");

        if (execute_command(clang_cmd) != 0) {
            // std::cerr << "Compilation to binary failed\n";
            LOG_ERROR("Compilation to binary failed");
            std::cout << "COMMAND " << clang_cmd << " LOG ERROR:";
            execute_command(clang_cmd);
            return false;
        }

        if (!fs::exists(bin_file) || fs::file_size(bin_file) == 0) {
            LOG_ERROR("Binary file \"%s\" not created", bin_file.c_str());
            return false;
        }

        return true;
    }

    /**
     * @brief Safe cleanup temp files
     *
     * @param output_base output base filename
     **/
    void cleanup_temp_files(const std::string& output_base) {
        auto safe_remove = [](const std::string& path) {
            try {
                if (fs::exists(path)) {
                    fs::remove(path);
                    LOG_DEBUG("Remove temp file: %s", path.c_str());
                }
            } catch (...) {
                LOG_WARN("Could not remove file \"%s\"", path.c_str());
            }
        };

        safe_remove(output_base + ".ll");
        safe_remove(output_base + "-opt.ll");
    }

    /**
     * @brief Check all utils is available
     *
     * @return true
     * @return false
     **/
    auto check_utils_available() -> bool {
        const std::vector<std::string> REQUIRED_PROGS = {"opt", "clang++"};

        for (const auto& util : REQUIRED_PROGS) {
            if (!is_util_available(util)) {
                LOG_ERROR("Required util \"%s\" not found. Install yourself.", util.c_str());
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Check is valid output name
     *
     * @param name output name
     * @return true
     * @return false
     **/
    auto is_valid_output_name(const std::string& name) -> bool {
        if (name.empty()) return false;

        const std::string FORBIDDEN_CHARS = "/\\:*?\"<>|";
        return std::none_of(name.begin(), name.end(), [&](char c) {
            return FORBIDDEN_CHARS.find(c) != std::string::npos;
        });
    }
}

/**
 * @brief Command Arguments Parser
 *
 **/
class InputParser{
    public:
        /**
         * @brief Construct a new Input Parser object
         *
         * @param argc arguments count
         * @param argv arguments list
         **/
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i) {
                this->m_TOKENS.emplace_back(argv[i]);
            }
        }

        /**
         * @brief Get the command option value
         *
         * @param option first short option (ex. -h)
         * @param option2 second long option (ex. --help)
         * @return const std::string&
         **/
        auto get_cmd_option(const std::string &option, const std::string &option2 = "") const -> const std::string&{
            std::vector<std::string>::const_iterator itr;
            itr =  boost::range::find(this->m_TOKENS, option);

            if (itr != this->m_TOKENS.end() && ++itr != this->m_TOKENS.end()){
                return *itr;
            }

            if (option2 != "") {
                std::vector<std::string>::const_iterator itr2;
                itr2 = boost::range::find(this->m_TOKENS, option2);

                if (itr2 != this->m_TOKENS.end() && ++itr2 != this->m_TOKENS.end()){
                    return *itr2;
                }
            }

            static const std::string EMPTY_STRING;

            return EMPTY_STRING;
        }

        /**
         * @brief Check option is exists
         *
         * @param option option name
         * @return true
         * @return false
         **/
        auto cmd_option_exists(const std::string &option) const -> bool{
            return boost::range::find(this->m_TOKENS, option)
                   != this->m_TOKENS.end();
        }
    private:
        std::vector <std::string> m_TOKENS;
};

/**
 * @brief Entrypoint
 *
 * @param argc args count
 * @param argv args list
 * @return int
 **/
auto main(int argc, char **argv) -> int {
    InputParser input(argc, argv);
    MorningLanguageLLVM morning_vm;

    std::string program;
    std::string output_base = "out";

    if (input.cmd_option_exists("-h") || input.cmd_option_exists("--help")) {
        print_help();
        return 0;
    }

    if (input.cmd_option_exists("--lint")) {
        const std::string filename = input.get_cmd_option("--lint");

        if (!fs::exists(filename)) {
            LOG_ERROR("File \"%s\" not found", filename.c_str());
            return 1;
        }

        std::ifstream file(filename);
        std::string program((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

        Linter linter;

        // Синтаксическая проверка
        auto syntax_errors = linter.check_syntax(program);
        if (!syntax_errors.empty()) {
            LOG_ERROR("Syntax errors in %s:", filename.c_str());
            for (const auto& err : syntax_errors) {
                LOG_ERROR("  %s", err.c_str());
            }
            return 1;
        }

        // Глубокий линтинг
        try {
            syntax::MorningLangGrammar parser;
            Exp ast = parser.parse("[scope " + program + "]");
            auto issues = linter.lint(ast);

            if (issues.empty()) {
                LOG_INFO("No lint issues found in %s", filename.c_str());
                return 0;
            }

            LOG_WARN("Lint issues in %s:", filename.c_str());
            for (const auto& issue : issues) {
                LOG_WARN("  %s", issue.c_str());
            }
            return 2; // Отдельный код возврата для предупреждений
        } catch (const std::exception& e) {
            LOG_ERROR("Linting failed: %s", e.what());
            return 1;
        }
    }

    if (input.cmd_option_exists("-o") || input.cmd_option_exists("--output")) {
        output_base = input.get_cmd_option("-o", "--output");

        if (!is_valid_output_name(output_base)) {
            LOG_ERROR("Invalid output name: %s", output_base.c_str());
            return 1;
        }
    }

    if (input.cmd_option_exists("-f") || input.cmd_option_exists("--file")) {
        const std::string FILENAME = input.get_cmd_option("-f", "--file");

        if (!fs::exists(FILENAME)) {
            LOG_ERROR("File \"%s\" not found", FILENAME.c_str());
            return 1;
        }

        std::ifstream program_file(FILENAME);
        if (!program_file.is_open()) {
            LOG_ERROR("Cannot open file \"%s\"", FILENAME.c_str());
            return 1;
        }

        std::stringstream buffer;
        buffer << program_file.rdbuf();
        program = buffer.str();

        if (program.empty()) {
            LOG_ERROR("File \"%s\" is empty", FILENAME.c_str());
            return 1;
        }
    } else if (input.cmd_option_exists("-e") || input.cmd_option_exists("--expression")) {
        program = input.get_cmd_option("-e", "--expression");

        if (program.empty()) {
            LOG_ERROR("Empty expression");
            return 1;
        }
    } else {
        LOG_ERROR("No input specified (use -e or -f)");
        print_help();
        return 1;
    }

    if (!check_utils_available()) {
        return 1;
    }

    try {
        LOG_INFO("Execute program...\n");
        morning_vm.execute(program, output_base);

        std::cout << "\n";

        const std::string LL_FILE = output_base + ".ll";
        if (!fs::exists(LL_FILE) || fs::file_size(LL_FILE) == 0) {
            LOG_ERROR("\nIR generation failed, no output file");
            return 1;
        }

        if (!compile_ir(output_base)) {
            LOG_ERROR("\nCompilation failed, temporary files retained for debugging");
            return 1;
        }

        cleanup_temp_files(output_base);

        LOG_INFO("Successfully compiled to %s", output_base.c_str());
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal " << e.what() << "\n";
        return 1;
    }

    return 0;
}
