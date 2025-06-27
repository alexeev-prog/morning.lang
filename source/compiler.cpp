#include "compiler.hpp"
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/CodeGen.h>
#include <lld/Common/Driver.h>
#include <lld/Common/LLVM.h>

llvm_compiler::llvm_compiler() {
    initialize_target();
}

auto llvm_compiler::initialize_target() -> bool {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string error;
    std::string triple = llvm::sys::getDefaultTargetTriple();
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);

    if (target == nullptr) {
        return false;
    }

    llvm::TargetOptions opt;
    target_machine.reset(target->createTargetMachine(
        triple,
        llvm::sys::getHostCPUName(),
        "",
        opt,
        std::nullopt,
        std::nullopt,
        llvm::CodeGenOptLevel::Default
    ));

    return target_machine != nullptr;
}

auto llvm_compiler::compile_module_to_object_file(llvm::Module& module, const std::string& output_filename) -> bool {
    if (target_machine == nullptr) {
        return false;
    }

    module.setDataLayout(target_machine->createDataLayout());
    module.setTargetTriple(target_machine->getTargetTriple().str());

    std::error_code file_error;
    llvm::raw_fd_ostream dest(output_filename, file_error, llvm::sys::fs::OF_None);
    if (file_error) {
        return false;
    }

    llvm::legacy::PassManager pass;
    llvm::CodeGenFileType file_type = llvm::CodeGenFileType::ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
        return false;
    }

    pass.run(module);
    dest.flush();
    return true;
}
