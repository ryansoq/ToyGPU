#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUREGISTERINFO_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUREGISTERINFO_H
#include "llvm/CodeGen/TargetRegisterInfo.h"
#define GET_REGINFO_HEADER
#include "ToyGPUGenRegisterInfo.inc"
namespace llvm {
struct ToyGPURegisterInfo : public ToyGPUGenRegisterInfo {
  ToyGPURegisterInfo();
  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
};
} // namespace llvm
#endif
