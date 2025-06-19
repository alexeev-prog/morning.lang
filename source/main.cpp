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

namespace fs = std::filesystem;

namespace {
    void print_help() {
        std::cout << "\nUsage: morningllvm [options]\n\n"
                  << "Options:\n"
                  << "    -e, --expression  Expression to parse\n"
                  << "    -f, --file        File to parse\n"
                  << "    -o, --output      Output binary name\n\n";
    }

    // Проверка доступности компилятора в системе
    bool is_compiler_available(const std::string& compiler) {
        #ifdef _WIN32
        std::string cmd = "where " + compiler + " >nul 2>nul";
        #else
        std::string cmd = "command -v " + compiler + " >/dev/null 2>&1";
        #endif
        return system(cmd.c_str()) == 0;
    }

    // Безопасное выполнение команд
    int execute_command(const std::string& cmd) {
        #ifdef _WIN32
        return system((cmd + " >nul 2>nul").c_str());
        #else
        return system((cmd + " >/dev/null 2>&1").c_str());
        #endif
    }

    // Обработка путей с кавычками для безопасности
    std::string safe_path(const std::string& path) {
        if (path.empty()) return "\"\"";
        if (path.find(' ') != std::string::npos) {
            return "\"" + path + "\"";
        }
        return path;
    }

    // Компиляция IR с проверками на каждом этапе
    bool compile_ir(const std::string& output_base) {
        const std::string ll_file = output_base + ".ll";
        const std::string opt_ll_file = output_base + "-opt.ll";
        const std::string bin_file = output_base;

        // Проверка существования исходного файла
        if (!fs::exists(ll_file)) {
            std::cerr << "Error: IR file not found: " << ll_file << "\n";
            return false;
        }

        // Оптимизация IR
        std::string opt_cmd = "opt " + safe_path(ll_file) +
                              " -O3 -S -o " + safe_path(opt_ll_file);

        if (execute_command(opt_cmd) != 0) {
            std::cerr << "Error: Optimization failed\n";
            return false;
        }

        // Проверка результата оптимизации
        if (!fs::exists(opt_ll_file) || fs::file_size(opt_ll_file) == 0) {
            std::cerr << "Error: Optimized IR file not created\n";
            return false;
        }

        // Компиляция в бинарник
        std::string clang_cmd = "clang++ -O3 " + safe_path(opt_ll_file) +
                                " -lgc -o " + safe_path(bin_file);

        if (execute_command(clang_cmd) != 0) {
            std::cerr << "Error: Compilation to binary failed\n";
            return false;
        }

        // Финальная проверка результата
        if (!fs::exists(bin_file) || fs::file_size(bin_file) == 0) {
            std::cerr << "Error: Binary file not created\n";
            return false;
        }

        return true;
    }

    // Безопасное удаление временных файлов
    void cleanup_temp_files(const std::string& output_base) {
        auto safe_remove = [](const std::string& path) {
            try {
                if (fs::exists(path)) {
                    fs::remove(path);
                }
            } catch (...) {
                std::cerr << "Warning: Could not remove file: " << path << "\n";
            }
        };

        safe_remove(output_base + ".ll");
        safe_remove(output_base + "-opt.ll");
    }

    // Проверка доступности всех необходимых компиляторов
    bool check_compilers_available() {
        const std::vector<std::string> required_compilers = {"opt", "clang++"};

        for (const auto& compiler : required_compilers) {
            if (!is_compiler_available(compiler)) {
                std::cerr << "Error: Required compiler not found: " << compiler << "\n";
                return false;
            }
        }
        return true;
    }

    // Проверка корректности имени выходного файла
    bool is_valid_output_name(const std::string& name) {
        if (name.empty()) return false;

        // Список запрещенных символов
        const std::string forbidden_chars = "/\\:*?\"<>|";
        return std::none_of(name.begin(), name.end(), [&](char c) {
            return forbidden_chars.find(c) != std::string::npos;
        });
    }
}
class InputParser{
    public:
        InputParser (int &argc, char **argv){
            for (int i=1; i < argc; ++i) {
                this->m_TOKENS.emplace_back(argv[i]);
            }
        }

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

        auto cmd_option_exists(const std::string &option) const -> bool{
            return boost::range::find(this->m_TOKENS, option)
                   != this->m_TOKENS.end();
        }
    private:
        std::vector <std::string> m_TOKENS;
};


auto main(int argc, char **argv) -> int {
    InputParser input(argc, argv);
    MorningLanguageLLVM morning_vm;
    std::string program;
    std::string output_base = "out";

    if (input.cmd_option_exists("-h") || input.cmd_option_exists("--help")) {
        print_help();
        return 0;
    }

    // Получение имени выходного файла
    if (input.cmd_option_exists("-o") || input.cmd_option_exists("--output")) {
        output_base = input.get_cmd_option("-o", "--output");

        if (!is_valid_output_name(output_base)) {
            std::cerr << "Error: Invalid output name: " << output_base << "\n";
            return 1;
        }
    }

    // Загрузка программы
    if (input.cmd_option_exists("-f") || input.cmd_option_exists("--file")) {
        const std::string FILENAME = input.get_cmd_option("-f", "--file");

        if (!fs::exists(FILENAME)) {
            std::cerr << "Error: File not found: " << FILENAME << "\n";
            return 1;
        }

        std::ifstream program_file(FILENAME);
        if (!program_file.is_open()) {
            std::cerr << "Error: Cannot open file: " << FILENAME << "\n";
            return 1;
        }

        std::stringstream buffer;
        buffer << program_file.rdbuf();
        program = buffer.str();

        if (program.empty()) {
            std::cerr << "Error: File is empty: " << FILENAME << "\n";
            return 1;
        }
    }
    else if (input.cmd_option_exists("-e") || input.cmd_option_exists("--expression")) {
        program = input.get_cmd_option("-e", "--expression");

        if (program.empty()) {
            std::cerr << "Error: Empty expression\n";
            return 1;
        }
    }
    else {
        std::cerr << "Error: No input specified (use -e or -f)\n";
        print_help();
        return 1;
    }

    // Проверка доступности компиляторов
    if (!check_compilers_available()) {
        return 1;
    }

    // Генерация и компиляция
    try {
        // Генерация IR
        morning_vm.execute(program, output_base);

        // Проверка сгенерированного IR
        const std::string ll_file = output_base + ".ll";
        if (!fs::exists(ll_file) || fs::file_size(ll_file) == 0) {
            std::cerr << "Error: IR generation failed, no output file\n";
            return 1;
        }

        // Компиляция в бинарник
        if (!compile_ir(output_base)) {
            std::cerr << "Compilation failed, temporary files retained for debugging\n";
            return 1;
        }

        // Успешная компиляция - очистка временных файлов
        cleanup_temp_files(output_base);

        std::cout << "Successfully compiled to: " << output_base << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
