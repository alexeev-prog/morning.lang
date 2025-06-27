#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

/**
 * @brief Command line option definition structure
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
 * Supports short and long options with unified handling,
 * automatic help generation, and strict validation.
 *
 * Designed following SOLID, ACID, KISS, and DRY principles.
 */
class InputParser {
public:
    /**
     * @brief Construct a new Input Parser object
     *
     * @param program_name Name of the program for help display
     * @param description Program description for help display
     */
    InputParser(std::string program_name, std::string description);

    /**
     * @brief Add a new command line option
     *
     * @param opt Option definition to add
     * @throws std::invalid_argument if option name is duplicated
     */
    auto add_option(const Option& opt) -> void;

    /**
     * @brief Parse command line arguments
     *
     * @param argc Argument count
     * @param argv Argument values
     * @return true if parsing succeeded
     * @return false if parsing failed (invalid arguments)
     */
    auto parse(int argc, char** argv) -> bool;

    /**
     * @brief Check if option was provided
     *
     * @param name Short or long option name
     * @return true if option exists in command line
     * @return false otherwise
     */
    auto has_option(const std::string& name) const -> bool;

    /**
     * @brief Get argument value for option
     *
     * @param name Short or long option name
     * @return std::optional<std::string> Argument value if exists
     */
    auto get_argument(const std::string& name) const -> std::optional<std::string>;

    /**
     * @brief Get positional arguments
     *
     * @return const std::vector<std::string>& List of positional arguments
     */
    auto get_positional_args() const -> const std::vector<std::string>&;

    /**
     * @brief Get parsing errors
     *
     * @return const std::vector<std::string>& List of parsing errors
     */
    auto get_errors() const -> const std::vector<std::string>&;

    /**
     * @brief Generate help message
     *
     * @return std::string Formatted help message
     */
    auto generate_help() const -> std::string;

private:
    auto get_option_index(const std::string& name) const -> std::optional<size_t>;
    auto is_option_registered(const std::string& name) const -> bool;
    auto reset_state() -> void;
    auto is_equals_syntax_option(const std::string& token) const -> bool;
    auto is_regular_option(const std::string& token) const -> bool;
    auto handle_equals_syntax(const std::string& token) -> void;
    auto handle_regular_option(const std::string& token, int& index, int argc, char** argv) -> bool;
    auto handle_positional_arg(const std::string& token) -> void;

    std::string m_PROGRAM_NAME;                 ///< Program name
    std::string m_DESCRIPTION;                  ///< Program description
    std::vector<Option> m_OPTIONS;              ///< Registered options
    std::map<std::string, size_t> m_SHORT_MAP;  ///< Short name to index mapping
    std::map<std::string, size_t> m_LONG_MAP;   ///< Long name to index mapping
    std::map<size_t, std::string> m_PARSED_VALUES;  ///< Parsed option values
    std::vector<std::string> m_POSITIONAL_ARGS; ///< Positional arguments
    std::vector<std::string> m_ERRORS;          ///< Parsing errors
};
