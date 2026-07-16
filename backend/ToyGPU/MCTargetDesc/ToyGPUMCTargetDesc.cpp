//===-- ToyGPUMCTargetDesc.cpp - MC 層工廠註冊 ----------------------------===//
// 把 tablegen 產生的 MCRegisterInfo / MCInstrInfo / MCSubtargetInfo
// 和我們的 AsmInfo / InstPrinter 註冊給 registry。
#include "ToyGPUMCTargetDesc.h"
#include "ToyGPUInstPrinter.h"
#include "ToyGPUMCAsmInfo.h"
#include "TargetInfo/ToyGPUTargetInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
using namespace llvm;

#define GET_INSTRINFO_MC_DESC
#include "ToyGPUGenInstrInfo.inc"
#define GET_REGINFO_MC_DESC
#include "ToyGPUGenRegisterInfo.inc"
#define GET_SUBTARGETINFO_MC_DESC
#include "ToyGPUGenSubtargetInfo.inc"

static MCRegisterInfo *createToyGPUMCRegisterInfo(const Triple &TT) {
  auto *X = new MCRegisterInfo();
  InitToyGPUMCRegisterInfo(X, /*RA=*/0);
  return X;
}
static MCInstrInfo *createToyGPUMCInstrInfo() {
  auto *X = new MCInstrInfo();
  InitToyGPUMCInstrInfo(X);
  return X;
}
static MCSubtargetInfo *createToyGPUMCSubtargetInfo(const Triple &TT,
                                                    StringRef CPU,
                                                    StringRef FS) {
  return createToyGPUMCSubtargetInfoImpl(TT, CPU.empty() ? "generic" : CPU,
                                         /*TuneCPU=*/"generic", FS);
}
static MCAsmInfo *createToyGPUMCAsmInfo(const MCRegisterInfo &MRI,
                                        const Triple &TT,
                                        const MCTargetOptions &Options) {
  return new ToyGPUMCAsmInfo(TT);
}
static MCInstPrinter *createToyGPUMCInstPrinter(const Triple &T,
                                                unsigned SyntaxVariant,
                                                const MCAsmInfo &MAI,
                                                const MCInstrInfo &MII,
                                                const MCRegisterInfo &MRI) {
  return new ToyGPUInstPrinter(MAI, MII, MRI);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeToyGPUTargetMC() {
  Target &T = getTheToyGPUTarget();
  TargetRegistry::RegisterMCRegInfo(T, createToyGPUMCRegisterInfo);
  TargetRegistry::RegisterMCInstrInfo(T, createToyGPUMCInstrInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createToyGPUMCSubtargetInfo);
  TargetRegistry::RegisterMCAsmInfo(T, createToyGPUMCAsmInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createToyGPUMCInstPrinter);
}
