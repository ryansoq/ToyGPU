#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUINSTRINFO_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUINSTRINFO_H
#include "ToyGPURegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#define GET_INSTRINFO_HEADER
#include "ToyGPUGenInstrInfo.inc"
namespace llvm {
class ToyGPUInstrInfo : public ToyGPUGenInstrInfo {
  const ToyGPURegisterInfo RI;
public:
  ToyGPUInstrInfo();
  const ToyGPURegisterInfo &getRegisterInfo() const { return RI; }
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &DL, MCRegister DestReg, MCRegister SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
};
} // namespace llvm
#endif
