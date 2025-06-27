#pragma once

#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>
#include <string>
#include <memory>
#include <vector>

class llvm_compiler {
public:
    llvm_compiler();
    auto compile_module_to_object_file(llvm::Module& module, const std::string& output_filename) -> bool;

private:
    std::unique_ptr<llvm::TargetMachine> target_machine;
    auto initialize_target() -> bool;
};
