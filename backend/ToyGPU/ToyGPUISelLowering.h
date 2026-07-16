#ifndef LLVM_LIB_TARGET_TOYGPU_TOYGPUISELLOWERING_H
#define LLVM_LIB_TARGET_TOYGPU_TOYGPUISELLOWERING_H
#include "llvm/CodeGen/TargetLowering.h"
namespace llvm {
class ToyGPUSubtarget;

namespace ToyGPUISD {
enum NodeType : unsigned {
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  RET_GLUE,       // return
};
} // namespace ToyGPUISD

class ToyGPUTargetLowering : public TargetLowering {
public:
  ToyGPUTargetLowering(const TargetMachine &TM, const ToyGPUSubtarget &STI);

  const char *getTargetNodeName(unsigned Opcode) const override;

  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CC, bool IsVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               const SDLoc &DL, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const override;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CC, bool IsVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
                      SelectionDAG &DAG) const override;
};
} // namespace llvm
#endif
