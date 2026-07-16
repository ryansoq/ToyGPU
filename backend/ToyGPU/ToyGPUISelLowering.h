#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUISELLOWERING_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUISELLOWERING_H
#include "llvm/ADT/APFloat.h"
#include "llvm/CodeGen/TargetLowering.h"
namespace llvm {
class ToyGPUSubtarget;

namespace ToyGPUISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET_GLUE,       // return
  LOAD_IN,        // @toygpu.input(i32)  -> LDIN
  LOAD_UNI,       // @toygpu.uniform(i32) -> LDUNI
  STORE_OUT,      // @toygpu.output(i32, f32) -> STOUT
};
} // namespace ToyGPUISD

class ToyGPUTargetLowering : public TargetLowering {
public:
  ToyGPUTargetLowering(const TargetMachine &TM, const ToyGPUSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

  // 讓所有 f32 immediate 都「合法」→ SelectionDAG 保留 fpimm 節點，
  // 由 LDI 的 pattern 承接，而不是 spill 到 constant pool（我們沒有
  // load-from-memory 指令，spill 會直接 Cannot select）。
  bool isFPImmLegal(const APFloat &, EVT, bool) const override { return true; }

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CC, bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CC, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;

  // 攔截 @toygpu.input/uniform/output 三個 call，換成 target node。
  SDValue LowerCall(CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const override;
};
} // namespace llvm
#endif
