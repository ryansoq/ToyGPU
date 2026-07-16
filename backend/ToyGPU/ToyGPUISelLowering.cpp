//===-- ToyGPUISelLowering.cpp - IR → SelectionDAG 的 target 規則 ---------===//
// C1 版：只教會 LLVM 兩件事 ——
//   1. f32 值住在 FPR class（addRegisterClass）
//   2. `ret void` 變成 ToyGPUISD::RET_GLUE 節點（td pattern 再配到 RET 指令）
#include "ToyGPUISelLowering.h"
#include "ToyGPUSubtarget.h"
#include "MCTargetDesc/ToyGPUMCTargetDesc.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Support/ErrorHandling.h"
using namespace llvm;

ToyGPUTargetLowering::ToyGPUTargetLowering(const TargetMachine &TM,
                                           const ToyGPUSubtarget &STI)
    : TargetLowering(TM) {
  addRegisterClass(MVT::f32, &ToyGPU::FPRRegClass);
  // i32 掛同一個 class：跟 .td 的 [f32, i32] 同理 ——
  // computeRegisterProperties assert 要求至少一個整數 register class。
  addRegisterClass(MVT::i32, &ToyGPU::FPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
}

const char *ToyGPUTargetLowering::getTargetNodeName(unsigned Opcode) const {
  switch ((ToyGPUISD::NodeType)Opcode) {
  case ToyGPUISD::RET_GLUE: return "ToyGPUISD::RET_GLUE";
  default: return nullptr;
  }
}

SDValue ToyGPUTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID, bool,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &, SelectionDAG &,
    SmallVectorImpl<SDValue> &) const {
  if (!Ins.empty())
    report_fatal_error("ToyGPU: shader main() takes no arguments");
  return Chain;
}

SDValue ToyGPUTargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID, bool,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (!Outs.empty())
    report_fatal_error("ToyGPU: shader main() returns void only");
  return DAG.getNode(ToyGPUISD::RET_GLUE, DL, MVT::Other, Chain);
}
