#ifndef LLVM_LIB_TARGET_TOYGPU_MCTARGETDESC_TOYGPUMCTARGETDESC_H
#define LLVM_LIB_TARGET_TOYGPU_MCTARGETDESC_TOYGPUMCTARGETDESC_H

// tablegen 產物的 enum（暫存器編號、指令 opcode）
#define GET_REGINFO_ENUM
#include "ToyGPUGenRegisterInfo.inc"
#define GET_INSTRINFO_ENUM
#include "ToyGPUGenInstrInfo.inc"
#define GET_SUBTARGETINFO_ENUM
#include "ToyGPUGenSubtargetInfo.inc"

#endif
