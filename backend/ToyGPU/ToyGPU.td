//===-- ToyGPU.td - ToyGPU target 總定義 --------------------*- tablegen -*-===//
// TableGen 的入口：include 官方 Target.td 框架，
// 再掛上我們的 register / instruction 定義。
include "llvm/Target/Target.td"

include "ToyGPURegisterInfo.td"
include "ToyGPUInstrInfo.td"

def ToyGPUInstrInfoDef : InstrInfo;

// 一個最簡處理器模型（沒有排程資訊）
def : ProcessorModel<"generic", NoSchedModel, []>;

def ToyGPU : Target {
  let InstructionSet = ToyGPUInstrInfoDef;
}
