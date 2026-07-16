//===-- ToyGPUTargetMachine.cpp - target 的「總開關」----------------------===//
// DataLayout 字串 + pass pipeline（我們只掛一個 ISel pass）。
#include "ToyGPUTargetMachine.h"
#include "ToyGPU.h"
#include "TargetInfo/ToyGPUTargetInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeToyGPUTarget() {
  RegisterTargetMachine<ToyGPUTargetMachine> X(getTheToyGPUTarget());
}

// e=little-endian, p:32, f32 對齊 32, n32=原生整數寬
static StringRef computeDataLayout() {
  return "e-m:e-p:32:32-i32:32-f32:32-n32";
}

ToyGPUTargetMachine::ToyGPUTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, std::optional<Reloc::Model> RM,
    std::optional<CodeModel::Model> CM, CodeGenOptLevel OL, bool)
    : CodeGenTargetMachineImpl(T, computeDataLayout(), TT, CPU, FS, Options,
                               RM.value_or(Reloc::Static),
                               CM.value_or(CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();
}

namespace {
class ToyGPUPassConfig : public TargetPassConfig {
public:
  ToyGPUPassConfig(ToyGPUTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}
  ToyGPUTargetMachine &getToyGPUTargetMachine() const {
    return getTM<ToyGPUTargetMachine>();
  }
  bool addInstSelector() override {
    addPass(createToyGPUISelDag(getToyGPUTargetMachine(), getOptLevel()));
    return false;
  }
};
} // namespace

TargetPassConfig *ToyGPUTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new ToyGPUPassConfig(*this, PM);
}
