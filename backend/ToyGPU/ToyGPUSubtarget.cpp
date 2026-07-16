#include "ToyGPUSubtarget.h"
using namespace llvm;
#define DEBUG_TYPE "toygpu-subtarget"
#define GET_SUBTARGETINFO_CTOR
#define GET_SUBTARGETINFO_TARGET_DESC
#include "ToyGPUGenSubtargetInfo.inc"

ToyGPUSubtarget::ToyGPUSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                                 const TargetMachine &TM)
    : ToyGPUGenSubtargetInfo(TT, CPU.empty() ? "generic" : CPU, "generic", FS),
      TLInfo(TM, *this) {}
