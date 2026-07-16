#ifndef LLVM_LIB_TARGET_TOYGPU_MCTARGETDESC_TOYGPUINSTPRINTER_H
#define LLVM_LIB_TARGET_TOYGPU_MCTARGETDESC_TOYGPUINSTPRINTER_H
#include "llvm/MC/MCInstPrinter.h"
namespace llvm {
// 把 MCInst 印成 toyasm 吃的文字組語。
// printInstruction/getRegisterName 由 tablegen 從 AsmString 產生。
class ToyGPUInstPrinter : public MCInstPrinter {
public:
  ToyGPUInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                    const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}

  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &O) override;
  void printRegName(raw_ostream &O, MCRegister Reg) override;

  // tablegen 產生（GenAsmWriter.inc）
  std::pair<const char *, uint64_t> getMnemonic(const MCInst &MI) const override;
  void printInstruction(const MCInst *MI, uint64_t Address, raw_ostream &O);
  static const char *getRegisterName(MCRegister Reg);

  // AsmString 裡的 operand 印法
  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
};
} // namespace llvm
#endif
