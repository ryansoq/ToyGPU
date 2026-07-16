//===-- ToyGPUInstPrinter.cpp - MCInst → toyasm 文字 ----------------------===//
#include "ToyGPUInstPrinter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

// tablegen 從每條指令的 AsmString 產生 printInstruction
#include "ToyGPUGenAsmWriter.inc"

void ToyGPUInstPrinter::printRegName(raw_ostream &O, MCRegister Reg) {
  O << getRegisterName(Reg);
}

void ToyGPUInstPrinter::printInst(const MCInst *MI, uint64_t Address,
                                  StringRef Annot, const MCSubtargetInfo &STI,
                                  raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void ToyGPUInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg()) { O << getRegisterName(Op.getReg()); return; }
  if (Op.isImm()) { O << Op.getImm(); return; }
  if (Op.isSFPImm()) {                       // LDI 的 float immediate
    float F;
    uint32_t Bits = Op.getSFPImm();
    memcpy(&F, &Bits, 4);
    O << F;
    return;
  }
  llvm_unreachable("unknown operand kind");
}
