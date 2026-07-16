//===-- ToyGPUInstrInfo.td - 指令定義 -----------------------*- tablegen -*-===//

// SelectionDAG 自訂節點：return。
// LowerReturn 會把 IR 的 ret 變成這個節點，再由下面的 pattern 配到 RET 指令。
def TGRetGlue : SDNode<"ToyGPUISD::RET_GLUE", SDTNone,
                       [SDNPHasChain, SDNPOptInGlue]>;

// I/U/O 存取的 target-specific SDNode（由 ISelLowering 產生）。
// LDIN/LDUNI：(chain, i32 index) -> f32；STOUT：(chain, i32 index, f32 val)。
def SDT_TGLoadSlot : SDTypeProfile<1, 1, [SDTCisVT<0, f32>, SDTCisVT<1, i32>]>;
def SDT_TGStoreSlot : SDTypeProfile<0, 2, [SDTCisVT<0, i32>, SDTCisVT<1, f32>]>;
def TGLoadIn  : SDNode<"ToyGPUISD::LOAD_IN",  SDT_TGLoadSlot,  [SDNPHasChain]>;
def TGLoadUni : SDNode<"ToyGPUISD::LOAD_UNI", SDT_TGLoadSlot,  [SDNPHasChain]>;
def TGStoreOut: SDNode<"ToyGPUISD::STORE_OUT", SDT_TGStoreSlot, [SDNPHasChain]>;

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


// ── 算術指令（C2）───────────────────────────────────────────────
// 三址浮點：dst = src1 op src2。td 的 pattern 直接把 IR 的 fadd/fsub/
// fmul 配到這裡 —— 這就是 instruction selection「把 IR 節點翻成機器
// 指令」的核心：一條 pattern = 一條翻譯規則。
class TGArith<bits<8> op, string mn, SDPatternOperator node>
    : TGInst<(outs FPR:$dst), (ins FPR:$src1, FPR:$src2),
             mn # " $dst, $src1, $src2",
             [(set FPR:$dst, (node FPR:$src1, FPR:$src2))]>;

def FADD : TGArith<0x03, "FADD", fadd>;
def FSUB : TGArith<0x04, "FSUB", fsub>;
def FMUL : TGArith<0x05, "FMUL", fmul>;

// ── 常數載入（LDI）─────────────────────────────────────────────
// f32 immediate 進暫存器。fpimm 是 SelectionDAG 的浮點常數節點；
// 我們用一個 target-specific 的 f32imm operand 承接（AsmPrinter 端
// 已能印 SFPImm）。
def tgf32imm : Operand<f32>, FPImmLeaf<f32, [{ return true; }]>;

def LDI : TGInst<(outs FPR:$dst), (ins tgf32imm:$imm),
                 "LDI $dst, $imm",
                 [(set FPR:$dst, fpimm:$imm)]> {
  let Size = 8;   // 指令 word + immediate word
}

// ── 特殊暫存器存取（C2）─────────────────────────────────────────
// index 是立即數 operand（不是暫存器），呼應 isa.md：I/U/O 對編譯器
// 是「LDIN/LDUNI/STOUT 的立即數 operand」而非可分配暫存器。
def LDIN : TGInst<(outs FPR:$dst), (ins i32imm:$idx),
                  "LDIN $dst, I$idx", [(set FPR:$dst, (TGLoadIn timm:$idx))]>;
def LDUNI : TGInst<(outs FPR:$dst), (ins i32imm:$idx),
                   "LDUNI $dst, U$idx", [(set FPR:$dst, (TGLoadUni timm:$idx))]>;
def STOUT : TGInst<(outs), (ins i32imm:$idx, FPR:$val),
                   "STOUT O$idx, $val", [(TGStoreOut timm:$idx, FPR:$val)]> {
  let hasSideEffects = 1;   // 寫 output register = 對外可觀察，不可 DCE
}

def RET : TGInst<(outs), (ins), "RET", [(TGRetGlue)]> {
  let isTerminator = 1;
  let isReturn = 1;
  let isBarrier = 1;
}

// MOV：register allocator / coalescer 的 copyPhysReg 用。
// 沒有 IR pattern —— 它只由 codegen 內部（copy 消解）產生。
def MOV : TGInst<(outs FPR:$dst), (ins FPR:$src), "MOV $dst, $src", []>;
