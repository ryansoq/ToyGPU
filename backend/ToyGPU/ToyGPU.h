#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPU_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPU_H
#include "llvm/Pass.h"
#include "llvm/Support/CodeGen.h"
namespace llvm {
class ToyGPUTargetMachine;
class FunctionPass;
FunctionPass *createToyGPUISelDag(ToyGPUTargetMachine &TM,
                                  CodeGenOptLevel OptLevel);
} // namespace llvm
#endif
