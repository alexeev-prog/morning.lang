#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "parser/MorningLangGrammar.h"

/**
 * @struct LintRule
 * @brief Represents a linting rule with metadata and checking logic
 */
struct LintRule {
    std::string code;    ///< Unique rule identifier (e.g. "W001")
    std::string description;    ///< Brief explanation of the rule
    std::string example;    ///< Example of correct code
    std::function<void(const Exp&, std::vector<std::string>&)> checker;    ///< Rule implementation
};

/**
 * @class Linter
 * @brief Static analyzer for MorningLang code with enhanced identifier checks
 */
class Linter {
  public:
    Linter();

    /**
     * @brief Register a new linting rule
     * @param rule Rule configuration to add
     */
    void addRule(const LintRule& rule);

    /**
     * @brief Analyze AST for code quality issues
     * @param ast Abstract Syntax Tree root node
     * @return List of found issues with correction suggestions
     */
    std::vector<std::string> lint(const Exp& ast);

    /**
     * @brief Validate code syntax without execution
     * @param code Source code to validate
     * @return List of syntax errors (empty if valid)
     */
    auto check_syntax(const std::string& code) -> std::vector<std::string>;

  private:
    std::vector<LintRule> rules_;    ///< Registered linting rules
    std::unordered_set<std::string> operators_;    ///< Valid operator symbols
    std::unordered_set<std::string> keywords_;    ///< Keywords
    std::unordered_map<std::string, int> symbolDeclarations_;    ///< Symbol tracking

    /**
     * @brief Recursive AST traversal for rule checking
     */
    void traverse_ast(const Exp& node, std::vector<std::string>& issues);

    /**
     * @brief Validate identifier name format
     * @param name Identifier to validate
     * @return true if valid, false otherwise
     */
    auto is_valid_identifier(const std::string& name) const -> bool;

    /**
     * @brief Convert to snake_case with correction suggestions
     * @param name Input identifier
     * @return Suggested corrected name
     */
    auto suggest_snake_case(const std::string& name) const -> std::string;
};
