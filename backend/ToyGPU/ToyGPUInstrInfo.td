//===-- ToyGPUInstrInfo.td - 指令定義 -----------------------*- tablegen -*-===//

// SelectionDAG 自訂節點：return。
// LowerReturn 會把 IR 的 ret 變成這個節點，再由下面的 pattern 配到 RET 指令。
def TGRetGlue : SDNode<"ToyGPUISD::RET_GLUE", SDTNone,
                       [SDNPHasChain, SDNPOptInGlue]>;

// 指令基底：固定 4 bytes（LDI 例外 8）。沒有 MCCodeEmitter（v1 出文字
// 組語、由 toyasm 轉 binary），所以不需要編碼欄位，只要 AsmString。
class TGInst<dag outs, dag ins, string asmstr, list<dag> pattern>
    : Instruction {
  let Namespace = "ToyGPU";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
  let Size = 4;
}

def RET : TGInst<(outs), (ins), "RET", [(TGRetGlue)]> {
  let isTerminator = 1;
  let isReturn = 1;
  let isBarrier = 1;
}

// MOV：register allocator / coalescer 的 copyPhysReg 用。
// 沒有 IR pattern —— 它只由 codegen 內部（copy 消解）產生。
def MOV : TGInst<(outs FPR:$dst), (ins FPR:$src), "MOV $dst, $src", []>;
