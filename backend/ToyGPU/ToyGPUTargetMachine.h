#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUTARGETMACHINE_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUTARGETMACHINE_H
#include "ToyGPUSubtarget.h"
#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/Target/TargetMachine.h"
#include <optional>
namespace llvm {
class ToyGPUTargetMachine : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  ToyGPUSubtarget Subtarget;
public:
  ToyGPUTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      std::optional<Reloc::Model> RM,
                      std::optional<CodeModel::Model> CM, CodeGenOptLevel OL,
                      bool JIT);
  const ToyGPUSubtarget *getSubtargetImpl(const Function &) const override {
    return &Subtarget;
  }
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};
} // namespace llvm
#endif
