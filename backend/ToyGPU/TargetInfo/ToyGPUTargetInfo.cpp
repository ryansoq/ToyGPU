//===-- ToyGPUTargetInfo.cpp - target 註冊 --------------------------------===//
// porting 第一步：向 LLVM 的 target registry 報到。
// 做完這步 `llc --version` 就會列出 toygpu。
#include "TargetInfo/ToyGPUTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheToyGPUTarget() {
  static Target TheToyGPUTarget;
  return TheToyGPUTarget;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeToyGPUTargetInfo() {
  RegisterTarget<Triple::toygpu, /*HasJIT=*/false> X(
      getTheToyGPUTarget(), "toygpu", "ToyGPU (software teaching GPU)",
      "ToyGPU");
}
