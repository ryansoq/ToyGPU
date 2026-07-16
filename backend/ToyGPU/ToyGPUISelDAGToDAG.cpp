//===-- ToyGPUISelDAGToDAG.cpp - DAG → 機器指令選擇 -----------------------===//
// SelectionDAGISel 的最小子類：所有選擇都交給 tablegen 從 .td pattern
// 產生的 SelectCode。之後若要手寫特殊選擇邏輯（addressing mode 等）
// 才需要在 Select() 加 case。
#include "ToyGPU.h"
#include "ToyGPUTargetMachine.h"
#include "MCTargetDesc/ToyGPUMCTargetDesc.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
using namespace llvm;

#define DEBUG_TYPE "toygpu-isel"
#define PASS_NAME "ToyGPU DAG->DAG Pattern Instruction Selection"

namespace {
class ToyGPUDAGToDAGISel : public SelectionDAGISel {
public:
  ToyGPUDAGToDAGISel(ToyGPUTargetMachine &TM, CodeGenOptLevel OL)
      : SelectionDAGISel(TM, OL) {}

  void Select(SDNode *N) override {
    if (N->isMachineOpcode()) { N->setNodeId(-1); return; }
    SelectCode(N);
  }

#include "ToyGPUGenDAGISel.inc"
};

class ToyGPUDAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  ToyGPUDAGToDAGISelLegacy(ToyGPUTargetMachine &TM, CodeGenOptLevel OL)
      : SelectionDAGISelLegacy(
            ID, std::make_unique<ToyGPUDAGToDAGISel>(TM, OL)) {}
};
char ToyGPUDAGToDAGISelLegacy::ID = 0;
} // namespace

FunctionPass *llvm::createToyGPUISelDag(ToyGPUTargetMachine &TM,
                                        CodeGenOptLevel OptLevel) {
  return new ToyGPUDAGToDAGISelLegacy(TM, OptLevel);
}
