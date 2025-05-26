#pragma once

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>

class MorningLanguageLLVM {
    public:
        MorningLanguageLLVM() { moduleInit(); }

        void exec(const std::string& program) {
            module->print(llvm::outs(), nullptr);

            saveModuleToFile("./out.ll");
        }
    
    private:
        void saveModuleToFile(const std::string& filename) {
            std::error_code errorCode;
            llvm::raw_fd_ostream outLL(filename, errorCode);
            module->print(outLL, nullptr);
        }

        void moduleInit() {
            ctx = std::make_unique<llvm::LLVMContext>();
            module = std::make_unique<llvm::Module>("MorningLangLLVM", *ctx);

            builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
        }

        std::unique_ptr<llvm::LLVMContext> ctx;

        std::unique_ptr<llvm::Module> module;

        std::unique_ptr<llvm::IRBuilder<>> builder;
};

