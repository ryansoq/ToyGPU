#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUSUBTARGET_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUSUBTARGET_H
#include "ToyGPUFrameLowering.h"
#include "ToyGPUISelLowering.h"
#include "ToyGPUInstrInfo.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#define GET_SUBTARGETINFO_HEADER
#include "ToyGPUGenSubtargetInfo.inc"
namespace llvm {
// Subtarget = 「這顆晶片有什麼」的集合體：指令集/暫存器/lowering/frame。
// ToyGPU 只有一種，全部直接內含。
class ToyGPUSubtarget : public ToyGPUGenSubtargetInfo {
  ToyGPUInstrInfo InstrInfo;
  ToyGPUTargetLowering TLInfo;
  ToyGPUFrameLowering FrameLowering;
  // SelectionDAG 在 instruction selection 期間會查這個（例如
  // mayRaiseFPException 對 target node 的判斷）—— 忘了提供會直接
  // segfault（base 回 nullptr、caller 不檢查）。極簡 target 的第三個坑。
  SelectionDAGTargetInfo TSInfo;
public:
  ToyGPUSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                  const TargetMachine &TM);
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  const ToyGPUInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const ToyGPURegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const ToyGPUTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const ToyGPUFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
};
} // namespace llvm
#endif
