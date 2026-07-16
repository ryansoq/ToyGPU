//===-- ToyGPUAsmPrinter.cpp - MachineInstr → MCInst → 文字組語 -----------===//
// AsmPrinter 是 codegen 的最後一站：把 MachineInstr 轉 MCInst，
// 交給 InstPrinter 印字。MO_FPImmediate（LDI 的 float）在這裡轉
// SFPImm bits。
#include "MCTargetDesc/ToyGPUMCTargetDesc.h"
#include "TargetInfo/ToyGPUTargetInfo.h"
#include "ToyGPUTargetMachine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

namespace {
class ToyGPUAsmPrinter : public AsmPrinter {
public:
  ToyGPUAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)) {}
  StringRef getPassName() const override { return "ToyGPU Assembly Printer"; }
  void emitInstruction(const MachineInstr *MI) override;
};
} // namespace

void ToyGPUAsmPrinter::emitInstruction(const MachineInstr *MI) {
  MCInst Inst;
  Inst.setOpcode(MI->getOpcode());
  for (const MachineOperand &MO : MI->operands()) {
    switch (MO.getType()) {
    case MachineOperand::MO_Register:
      if (MO.isImplicit()) continue;
      Inst.addOperand(MCOperand::createReg(MO.getReg()));
      break;
    case MachineOperand::MO_Immediate:
      Inst.addOperand(MCOperand::createImm(MO.getImm()));
      break;
    case MachineOperand::MO_FPImmediate: {           // LDI f32
      APInt Bits = MO.getFPImm()->getValueAPF().bitcastToAPInt();
      Inst.addOperand(MCOperand::createSFPImm(Bits.getZExtValue()));
      break;
    }
    default:
      llvm_unreachable("unhandled machine operand type");
    }
  }
  EmitToStreamer(*OutStreamer, Inst);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeToyGPUAsmPrinter() {
  RegisterAsmPrinter<ToyGPUAsmPrinter> X(getTheToyGPUTarget());
}
