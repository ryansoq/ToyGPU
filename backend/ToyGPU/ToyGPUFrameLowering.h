#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUFRAMELOWERING_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUFRAMELOWERING_H
#include "llvm/CodeGen/TargetFrameLowering.h"
namespace llvm {
// 沒有 stack → prologue/epilogue 都是空的。
class ToyGPUFrameLowering : public TargetFrameLowering {
public:
  ToyGPUFrameLowering()
      : TargetFrameLowering(StackGrowsDown, Align(4), 0) {}
  void emitPrologue(MachineFunction &, MachineBasicBlock &) const override {}
  void emitEpilogue(MachineFunction &, MachineBasicBlock &) const override {}
protected:
  bool hasFPImpl(const MachineFunction &) const override { return false; }
};
} // namespace llvm
#endif
