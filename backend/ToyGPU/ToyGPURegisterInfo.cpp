//===-- ToyGPURegisterInfo.cpp --------------------------------------------===//
// v1 沒有 stack、沒有 calling convention 可言（只有無參數的 main）：
// callee-saved 清單是空的、frame index 直接報錯（spec：spill 大聲死）。
#include "ToyGPURegisterInfo.h"
#include "MCTargetDesc/ToyGPUMCTargetDesc.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "ToyGPUFrameLowering.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

#define GET_REGINFO_TARGET_DESC
#include "ToyGPUGenRegisterInfo.inc"

ToyGPURegisterInfo::ToyGPURegisterInfo() : ToyGPUGenRegisterInfo(/*RA=*/0) {}

const MCPhysReg *
ToyGPURegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  static const MCPhysReg None[] = {0};
  return None;
}

BitVector ToyGPURegisterInfo::getReservedRegs(const MachineFunction &MF) const {
  return BitVector(getNumRegs());          // 全部可分配
}

bool ToyGPURegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator,
                                             int, unsigned,
                                             RegScavenger *) const {
  report_fatal_error("ToyGPU v1 has no stack: spill/frame access not supported "
                     "(shader too complex for 16 registers?)");
}

Register ToyGPURegisterInfo::getFrameRegister(const MachineFunction &) const {
  return ToyGPU::R15;                      // 佔位；沒有 frame 所以用不到
}
