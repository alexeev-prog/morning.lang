#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "llvm/IR/Value.h"
#include "logger.hpp"
#include "tracelogger.hpp"

class Environment : public std::enable_shared_from_this<Environment> {
  public:
    Environment(std::map<std::string, llvm::Value*> record, std::shared_ptr<Environment> parent)
        : m_RECORD(std::move(record))
        , m_PARENT(std::move(parent)) {
        LOG_TRACE
    }

    auto define(const std::string& var_name, llvm::Value* value) -> llvm::Value* {
        LOG_TRACE

        m_RECORD[var_name] = value;

        return value;
    }

    auto lookup_by_name(const std::string& var_name) -> llvm::Value* {
        LOG_TRACE

        return resolve(var_name)->m_RECORD[var_name];
    }

  private:
    auto resolve(const std::string& name) -> std::shared_ptr<Environment> {
        LOG_TRACE

        if (m_RECORD.find(name) != m_RECORD.end()) {
            return shared_from_this();
        }

        if (m_PARENT == nullptr) {
            LOG_CRITICAL("Variable \"%s\" is not defined", name.c_str());
            return nullptr;    // Never reached but for safety
        }

        return m_PARENT->resolve(name);
    }

    std::map<std::string, llvm::Value*> m_RECORD;

    std::shared_ptr<Environment> m_PARENT;
};
