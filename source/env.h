#pragma once

#include <map>
#include <memory>
#include <utility>
#include <string>
#include "llvm/IR/Value.h"

#include "logger.h"

class Environment : public std::enable_shared_from_this<Environment> {
    public:
        Environment(std::map<std::string, llvm::Value*> record,
                    std::shared_ptr<Environment> parent)
                    : m_RECORD(std::move(record)), m_PARENT(std::move(parent)) {}

        llvm::Value* define(const std::string& var_name, llvm::Value* value) {
            m_RECORD[var_name] = value;

            return value;
        }

        llvm::Value* lookup_by_name(const std::string& var_name) {
            return resolve(var_name)->m_RECORD[var_name];
        }

    private:
        std::shared_ptr<Environment> resolve(const std::string& name) {
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
