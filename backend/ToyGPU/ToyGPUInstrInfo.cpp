//===-- ToyGPUInstrInfo.cpp -----------------------------------------------===//
#include "ToyGPUInstrInfo.h"
#include "ToyGPU.h"
#include "MCTargetDesc/ToyGPUMCTargetDesc.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "ToyGPUGenInstrInfo.inc"

ToyGPUInstrInfo::ToyGPUInstrInfo() : ToyGPUGenInstrInfo() {}

// register allocator / coalescer 需要「怎麼複製一個暫存器」→ MOV
void ToyGPUInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator I,
                                  const DebugLoc &DL, MCRegister DestReg,
                                  MCRegister SrcReg, bool KillSrc,
                                  bool /*RenamableDest*/,
                                  bool /*RenamableSrc*/) const {
  BuildMI(MBB, I, DL, get(ToyGPU::MOV), DestReg)
      .addReg(SrcReg, getKillRegState(KillSrc));
}
