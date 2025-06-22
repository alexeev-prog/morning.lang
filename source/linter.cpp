#include "linter.hpp"
#include "logger.hpp"
#include <cctype>
#include <algorithm>

// Initialize with valid operators
Linter::Linter() : operators_{"+", "-", "*", "/", ">", "<", ">=", "<=", "==", "!="},
                   keywords_{"func", "scope", "fprint", "check", "if", "elif", "else",
                            "finput", "while", "loop", "for", "set", "var", "const",
                            "!int", "!int64", "!int32", "!int16", "!int8", "!bool",
                            "!none", "!str"} {
    // Rule: Valid identifier format
    addRule({
        "W001",
        "Identifiers must contain only letters, digits and underscores",
        "my-var -> my_var",
        [this](const Exp& exp, std::vector<std::string>& issues) {
            if (exp.type != ExpType::SYMBOL) return;

            const std::string& name = exp.string;

            // Skip operators and special forms
            if (operators_.count(name) > 0 ||
                name == "func" || name == "var" || name == "const" ||
                name == "if" || name == "while" || name == "for") {
                return;
            }

            if (!is_valid_identifier(name)) {
                std::string suggestion;
                if (name.find('-') != std::string::npos) {
                    // Replace hyphens with underscores
                    suggestion = name;
                    std::replace(suggestion.begin(), suggestion.end(), '-', '_');
                } else {
                    // Remove invalid characters
                    suggestion.reserve(name.size());
                    for (char c : name) {
                        if (std::isalnum(c) || c == '_') suggestion += c;
                    }
                    if (suggestion.empty()) suggestion = "valid_name";
                }

                issues.push_back(
                    "W001: Invalid identifier '" + name + "'\n"
                    "  Contains invalid characters (only a-z, 0-9, _ allowed)\n"
                    "  Suggested fix: use '" + suggestion + "' instead"
                );
            }
        }
    });

    // Rule: Snake case naming convention
    addRule({
        "W002",
        "Identifiers must use snake_case formatting",
        "myVariable -> my_variable",
        [this](const Exp& exp, std::vector<std::string>& issues) {
            if (exp.type != ExpType::SYMBOL) return;

            const std::string& name = exp.string;
            if (operators_.count(name) > 0 || !is_valid_identifier(name)) return;

            bool has_uppercase = std::any_of(name.begin(), name.end(),
                [](char c) { return std::isupper(c); });

            if (has_uppercase) {
                std::string suggestion = suggest_snake_case(name);
                issues.push_back(
                    "W002: Not snake_case: '" + name + "'\n"
                    "  Suggested fix: use '" + suggestion + "' instead\n"
                    "  Example: [var " + suggestion + " 10]"
                );
            }
        }
    });

    // Rule: Minimum identifier length
    addRule({
        "W003",
        "Identifiers must be at least 3 characters long",
        "x -> value_x",
        [this](const Exp& exp, std::vector<std::string>& issues) {
            if (exp.type != ExpType::SYMBOL) return;

            const std::string& name = exp.string;
            if (operators_.count(name) > 0 || !is_valid_identifier(name)) return;

            if (name.size() < 3) {
                std::string suggestion = name + "_value";
                issues.push_back(
                    "W003: Identifier too short: '" + name + "' (" +
                    std::to_string(name.size()) + " chars)\n"
                    "  Suggested fix: use '" + suggestion + "' instead\n"
                    "  Example: [var " + suggestion + " 10]"
                );
            }
        }
    });

    // Rule: Duplicate declarations
    addRule({
        "W004",
        "Duplicate symbol declaration in same scope",
        "Unique names for variables/functions",
        [this](const Exp& exp, std::vector<std::string>& issues) {
            if (exp.type != ExpType::LIST || exp.list.empty()) return;

            const Exp& head = exp.list[0];
            if (head.type != ExpType::SYMBOL) return;

            if (head.string == "func" || head.string == "var" || head.string == "const") {
                // Extract declared symbol name
                std::string name;
                if (exp.list.size() > 1) {
                    const Exp& nameExp = exp.list[1];
                    if (nameExp.type == ExpType::SYMBOL) {
                        name = nameExp.string;
                    } else if (nameExp.type == ExpType::LIST &&
                              !nameExp.list.empty() &&
                              nameExp.list[0].type == ExpType::SYMBOL) {
                        name = nameExp.list[0].string;
                    }
                }

                // Report duplicates with suggestion
                if (!name.empty() && is_valid_identifier(name)) {
                    if (symbolDeclarations_[name]++ > 0) {
                        std::string suggestion = name + "_2";
                        issues.push_back(
                            "W004: Duplicate declaration: '" + name + "'\n"
                            "  Suggested fix: rename to '" + suggestion + "'\n"
                            "  Example: [var " + suggestion + " value]"
                        );
                    }
                }
            }
        }
    });
}

auto Linter::is_valid_identifier(const std::string& name) const -> bool {
    if (name.empty()) return false;

    // First character must be letter or underscore
    if (!std::isalpha(name[0]) && name[0] != '_') {
        return false;
    }

    // Subsequent characters must be alphanumeric or underscore
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalnum(c) || c == '_';
    });
}

auto Linter::suggest_snake_case(const std::string& name) const -> std::string {
    std::string suggestion;
    suggestion.reserve(name.size() * 1.5); // Estimate size

    for (size_t i = 0; i < name.size(); ++i) {
        char c = name[i];

        // Convert to lowercase
        if (std::isupper(c)) {
            // Add underscore before uppercase (except first char)
            if (i > 0 && !std::isspace(suggestion.back()) &&
                suggestion.back() != '_') {
                suggestion += '_';
            }
            suggestion += std::tolower(c);
        }
        // Convert hyphen to underscore
        else if (c == '-') {
            suggestion += '_';
        }
        // Skip invalid characters
        else if (std::isalnum(c) || c == '_') {
            suggestion += c;
        }
    }

    // Remove consecutive underscores
    size_t pos;
    while ((pos = suggestion.find("__")) != std::string::npos) {
        suggestion.replace(pos, 2, "_");
    }

    // Remove leading/trailing underscores
    if (!suggestion.empty() && suggestion.front() == '_') {
        suggestion.erase(0, 1);
    }
    if (!suggestion.empty() && suggestion.back() == '_') {
        suggestion.pop_back();
    }

    return suggestion.empty() ? "valid_name" : suggestion;
}

void Linter::addRule(const LintRule& rule) {
    rules_.push_back(rule);
}

std::vector<std::string> Linter::lint(const Exp& ast) {
    std::vector<std::string> issues;
    symbolDeclarations_.clear(); // Reset state for new lint run
    traverse_ast(ast, issues);
    return issues;
}

void Linter::traverse_ast(const Exp& node, std::vector<std::string>& issues) {
    // Apply all rules to current node
    for (const auto& rule : rules_) {
        rule.checker(node, issues);
    }

    // Recursively process child nodes
    if (node.type == ExpType::LIST) {
        for (const auto& child : node.list) {
            traverse_ast(child, issues);
        }
    }
}

auto Linter::check_syntax(const std::string& code) -> std::vector<std::string> {
    syntax::MorningLangGrammar parser;
    try {
        // Wrap in scope for valid top-level expression
        parser.parse("[scope " + code + "]");
        return {};
    } catch (const std::exception& e) {
        return {"E001: Syntax error: " + std::string(e.what())};
    }
}
