#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "llvm/IR/Value.h"
#include "logger.h"

class Environment : public std::enable_shared_from_this<Environment> {
  public:
    Environment(std::map<std::string, llvm::Value*> record, std::shared_ptr<Environment> parent)
        : m_RECORD(std::move(record))
        , m_PARENT(std::move(parent)) {}

    auto define(const std::string& var_name, llvm::Value* value) -> llvm::Value* {
        m_RECORD[var_name] = value;

        return value;
    }

    auto lookup_by_name(const std::string& var_name) -> llvm::Value* {
        return resolve(var_name)->m_RECORD[var_name];
    }

  private:
    auto resolve(const std::string& name) -> std::shared_ptr<Environment> {
        if (m_RECORD.count(name) != 0) {
            return shared_from_this();
        }

        if (m_PARENT == nullptr) {
            DIE << "Varible \"" << name << "\" is not defined.\n";
        }

        return m_PARENT->resolve(name);
    }

    std::map<std::string, llvm::Value*> m_RECORD;

    std::shared_ptr<Environment> m_PARENT;
};
