#ifndef LLVM_LIB_TARGET_TOYGPU_MCTARGETDESC_TOYGPUMCASMINFO_H
#define LLVM_LIB_TARGET_TOYGPU_MCTARGETDESC_TOYGPUMCASMINFO_H
#include "llvm/MC/MCAsmInfoELF.h"
namespace llvm {
class Triple;
// 組語「風格」設定：註解字元、對齊指示等。
// 我們的 toyasm 用 ';' 當註解，跟這裡對齊。
class ToyGPUMCAsmInfo : public MCAsmInfoELF {
public:
  explicit ToyGPUMCAsmInfo(const Triple &TT) {
    CommentString = ";";
    HasSingleParameterDotFile = false;
    HasDotTypeDotSizeDirective = false;
    HasFunctionAlignment = false;
  }
};
} // namespace llvm
#endif
