#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include "morningllvm.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>

#include "logger.hpp"
#include "linter.hpp"

namespace fs = std::filesystem;

/**
 * @brief Command line option definition
 */
struct Option {
    std::string short_name;         ///< Short option name (e.g., "-h")
    std::string long_name;          ///< Long option name (e.g., "--help")
    std::string description;        ///< Option description for help
    bool requires_argument;         ///< Whether option requires an argument
    std::string arg_placeholder;    ///< Argument placeholder for help
};

/**
 * @brief Advanced command line arguments parser
 *
 * Supports both short and long options with unified handling,
 * automatic help generation, and strict validation.
 */
class InputParser {
public:
    /**
     * @brief Construct a new Input Parser object
     *
     * @param program_name Name of the program for help display
     * @param description Program description for help display
     */
    InputParser(std::string program_name, std::string description)
        : m_program_name(std::move(program_name)),
          m_description(std::move(description)) {}

    /**
     * @brief Add a new command line option
     *
     * @param opt Option definition to add
     */
    void add_option(const Option& opt) {
        const size_t idx = m_options.size();
        m_options.push_back(opt);

        if (!opt.short_name.empty()) {
            m_short_to_canonical[opt.short_name] = idx;
        }
        if (!opt.long_name.empty()) {
            m_long_to_canonical[opt.long_name] = idx;
        }
    }

    /**
     * @brief Parse command line arguments
     *
     * @param argc Argument count
     * @param argv Argument values
     * @return true if parsing succeeded
     * @return false if parsing failed (invalid arguments)
     */
    bool parse(int argc, char** argv) {
        m_parsed_options.clear();
        m_positional_args.clear();
        m_errors.clear();

        for (int i = 1; i < argc; ++i) {
            std::string token = argv[i];

            // Handle long options with '=' syntax
            if (token.size() >= 2 && token.substr(0, 2) == "--" && token.find('=') != std::string::npos) {
                size_t pos = token.find('=');
                std::string opt = token.substr(0, pos);
                std::string arg = token.substr(pos + 1);

                if (auto it = m_long_to_canonical.find(opt); it != m_long_to_canonical.end()) {
                    const auto& option = m_options[it->second];
                    if (option.requires_argument) {
                        m_parsed_options[it->second] = arg;
                    } else {
                        m_errors.push_back("Option " + opt + " doesn't accept arguments");
                    }
                } else {
                    m_errors.push_back("Unknown option: " + opt);
                }
                continue;
            }

            // Handle regular options
            if (!token.empty() && token[0] == '-') {
                size_t idx = 0;
                bool found = false;

                if (token.size() >= 2 && token.substr(0, 2) == "--") {
                    if (auto it = m_long_to_canonical.find(token); it != m_long_to_canonical.end()) {
                        idx = it->second;
                        found = true;
                    }
                } else {
                    if (auto it = m_short_to_canonical.find(token); it != m_short_to_canonical.end()) {
                        idx = it->second;
                        found = true;
                    }
                }

                if (!found) {
                    m_errors.push_back("Unknown option: " + token);
                    continue;
                }

                const auto& option = m_options[idx];
                if (option.requires_argument) {
                    if (i + 1 >= argc) {
                        m_errors.push_back("Missing argument for: " + token);
                    } else {
                        m_parsed_options[idx] = argv[++i];
                    }
                } else {
                    m_parsed_options[idx] = "";
                }
            } else {
                m_positional_args.push_back(token);
            }
        }

        return m_errors.empty();
    }

    /**
     * @brief Check if option was provided
     *
     * @param option_name Short or long option name
     * @return true if option exists in command line
     * @return false otherwise
     */
    bool has_option(const std::string& option_name) const {
        auto idx = get_option_index(option_name);
        return idx && m_parsed_options.find(*idx) != m_parsed_options.end();
    }

    /**
     * @brief Get argument value for option
     *
     * @param option_name Short or long option name
     * @return std::optional<std::string> Argument value if exists
     */
    std::optional<std::string> get_argument(const std::string& option_name) const {
        auto idx = get_option_index(option_name);
        if (!idx) return std::nullopt;

        auto it = m_parsed_options.find(*idx);
        if (it == m_parsed_options.end()) return std::nullopt;

        return it->second;
    }

    /**
     * @brief Get positional arguments
     *
     * @return const std::vector<std::string>& List of positional arguments
     */
    const std::vector<std::string>& get_positional_args() const {
        return m_positional_args;
    }

    /**
     * @brief Get parsing errors
     *
     * @return const std::vector<std::string>& List of parsing errors
     */
    const std::vector<std::string>& get_errors() const {
        return m_errors;
    }

    /**
     * @brief Generate help message
     *
     * @return std::string Formatted help message
     */
    std::string generate_help() const {
        std::ostringstream oss;
        oss << "Usage: " << m_program_name << " [options]\n\n";
        oss << m_description << "\n\n";
        oss << "Options:\n";

        for (const auto& opt : m_options) {
            std::string option_display;
            if (!opt.short_name.empty() && !opt.long_name.empty()) {
                option_display = opt.short_name + ", " + opt.long_name;
            } else if (!opt.short_name.empty()) {
                option_display = opt.short_name;
            } else {
                option_display = opt.long_name;
            }

            if (opt.requires_argument) {
                option_display += " " + opt.arg_placeholder;
            }

            oss << "  " << std::left << std::setw(30) << option_display
                << " " << opt.description << "\n";
        }

        return oss.str();
    }

private:
    std::optional<size_t> get_option_index(const std::string& name) const {
        if (name.size() >= 2 && name.substr(0, 2) == "--") {
            if (auto it = m_long_to_canonical.find(name); it != m_long_to_canonical.end()) {
                return it->second;
            }
        } else if (!name.empty() && name[0] == '-') {
            if (auto it = m_short_to_canonical.find(name); it != m_short_to_canonical.end()) {
                return it->second;
            }
        }

        // Try to find by alternate name
        if (auto it = m_long_to_canonical.find(name); it != m_long_to_canonical.end()) {
            return it->second;
        }
        if (auto it = m_short_to_canonical.find(name); it != m_short_to_canonical.end()) {
            return it->second;
        }

        return std::nullopt;
    }

    std::string m_program_name;
    std::string m_description;
    std::vector<Option> m_options;
    std::map<std::string, size_t> m_short_to_canonical;
    std::map<std::string, size_t> m_long_to_canonical;
    std::map<size_t, std::string> m_parsed_options;
    std::vector<std::string> m_positional_args;
    std::vector<std::string> m_errors;
};

namespace {
    auto launch_lint(const std::string& filename) -> int {
        if (!fs::exists(filename)) {
            LOG_ERROR("File \"%s\" not found", filename.c_str());
            return 1;
        }

        std::ifstream file(filename);
        std::string program((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

        Linter linter;

        // Syntax checking
        auto syntax_errors = linter.check_syntax(program);
        if (!syntax_errors.empty()) {
            LOG_ERROR("Syntax errors in %s:", filename.c_str());
            for (const auto& err : syntax_errors) {
                LOG_ERROR("  %s", err.c_str());
            }
            return 1;
        }

        // Deep linting
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
            return 2; // Separate exit code for warnings
        } catch (const std::exception& e) {
            LOG_ERROR("Linting failed: %s", e.what());
            return 1;
        }
    }

    /**
     * @brief Check if util is available (cross-platform)
     *
     * @param util util name
     * @return true if available
     * @return false if not available
     */
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
     * @param cmd command to execute
     * @param quiet suppress output
     * @return int exit code
     */
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
     * @brief Generate safe quoted path
     *
     * @param path raw path
     * @return std::string safe quoted path
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
     *
     * @param output_base output base filename
     * @return true if compilation succeeded
     * @return false if compilation failed
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
     *
     * @param output_base output base filename
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
     *
     * @return true if all available
     * @return false if any missing
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
     *
     * @param name output name
     * @return true if valid
     * @return false if invalid
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
 *
 * @param argc Argument count
 * @param argv Argument values
 * @return int Exit code
 */
auto main(int argc, char **argv) -> int {
    MorningLanguageLLVM morning_vm;
    std::string program;
    std::string output_base = "out";

    // Initialize parser with program info
    InputParser parser(
        fs::path(argv[0]).filename().string(),
        "MorningLLVM - Compiler for the Morning programming language"
    );

    // Register command line options
    parser.add_option({"-h", "--help", "Print this help message", false, ""});
    parser.add_option({"-e", "--expression", "Expression to parse", true, "<expr>"});
    parser.add_option({"-f", "--file", "File to parse", true, "<file>"});
    parser.add_option({"-l", "--lint", "File to lint", true, "<file>"});
    parser.add_option({"-o", "--output", "Output binary name", true, "<name>"});
    parser.add_option({"-k", "--keep", "Keep temporary files", false, ""});

    // Parse command line
    if (!parser.parse(argc, argv)) {
        for (const auto& error : parser.get_errors()) {
            LOG_ERROR("%s", error.c_str());
        }
        std::cerr << parser.generate_help() << std::endl;
        return 1;
    }

    // Handle help option
    if (parser.has_option("-h") || parser.has_option("--help")) {
        std::cout << parser.generate_help() << std::endl;
        return 0;
    }

    // Handle lint option
    if (parser.has_option("--lint") || parser.has_option("-l")) {
        if (auto filename = parser.get_argument("--lint")) {
            return launch_lint(*filename);
        } else {
            LOG_ERROR("Missing filename for --lint");
            return 1;
        }
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
        std::cerr << parser.generate_help() << std::endl;
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
        LOG_ERROR("Fatal error: %s", e.what());
        return 1;
    }

    return 0;
}
