//===-- ToyGPUISelLowering.cpp - IR → SelectionDAG 的 target 規則 ---------===//
// C1 版：只教會 LLVM 兩件事 ——
//   1. f32 值住在 FPR class（addRegisterClass）
//   2. `ret void` 變成 ToyGPUISD::RET_GLUE 節點（td pattern 再配到 RET 指令）
#include "ToyGPUISelLowering.h"
#include "ToyGPUSubtarget.h"
#include "MCTargetDesc/ToyGPUMCTargetDesc.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/IR/GlobalValue.h"
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
  case ToyGPUISD::LOAD_IN:   return "ToyGPUISD::LOAD_IN";
  case ToyGPUISD::LOAD_UNI:  return "ToyGPUISD::LOAD_UNI";
  case ToyGPUISD::STORE_OUT: return "ToyGPUISD::STORE_OUT";
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

//===----------------------------------------------------------------------===//
// LowerCall —— 這個 backend 的精華 30 行。
//
// shader 裡對 @toygpu.input/uniform/output 的呼叫，在 SelectionDAG 建構
// 期會走到這裡。我們不做「真的呼叫」（沒有 call ABI），而是攔截 callee
// 名字，換成對應的 target SDNode（LOAD_IN/LOAD_UNI/STORE_OUT）。td 的
// pattern 再把這些 node 配到 LDIN/LDUNI/STOUT 指令。
//
// 關鍵約束：index 參數必須是編譯期常數（ISA 的 I/U/O 編號是立即數）。
// 非常數 index 直接大聲報錯 —— 呼應全案「出錯要大聲」。
//===----------------------------------------------------------------------===//
SDValue ToyGPUTargetLowering::LowerCall(CallLoweringInfo &CLI,
                                        SmallVectorImpl<SDValue> &InVals) const {
  SelectionDAG &DAG = CLI.DAG;
  SDLoc DL = CLI.DL;
  SDValue Chain = CLI.Chain;

  // 取 callee 名字
  auto *GA = dyn_cast<GlobalAddressSDNode>(CLI.Callee);
  StringRef Name = GA ? GA->getGlobal()->getName() : StringRef();

  auto constIndex = [&](const SDValue &V) -> SDValue {
    auto *C = dyn_cast<ConstantSDNode>(V);
    if (!C)
      report_fatal_error("ToyGPU: @toygpu.input/uniform/output index must be "
                         "a compile-time constant");
    return DAG.getTargetConstant(C->getZExtValue(), DL, MVT::i32);
  };

  if (Name == "toygpu.input" || Name == "toygpu.uniform") {
    SDValue Idx = constIndex(CLI.OutVals[0]);
    unsigned Op = (Name == "toygpu.input") ? ToyGPUISD::LOAD_IN
                                           : ToyGPUISD::LOAD_UNI;
    SDVTList VTs = DAG.getVTList(MVT::f32, MVT::Other);
    SDValue R = DAG.getNode(Op, DL, VTs, {Chain, Idx});
    InVals.push_back(R);                 // f32 結果
    return R.getValue(1);                // 新的 chain
  }
  if (Name == "toygpu.output") {
    SDValue Idx = constIndex(CLI.OutVals[0]);
    SDValue Val = CLI.OutVals[1];        // f32
    return DAG.getNode(ToyGPUISD::STORE_OUT, DL, MVT::Other, {Chain, Idx, Val});
  }
  report_fatal_error("ToyGPU: only @toygpu.input/uniform/output calls allowed "
                     "(no general call ABI)");
}
