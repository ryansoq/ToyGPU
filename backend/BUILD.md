# ToyGPU LLVM Backend — build 說明

target 原始碼放本 repo `backend/ToyGPU/`（好讀好 review），
用 symlink 掛進 llvm-project，是 out-of-tree 原始碼 + in-tree build 的混合式。

```bash
bash backend/setup-llvm.sh      # clone llvm 20.1.8 + patch Triple + symlink + cmake
ninja -C ~/llvm-project/build llc   # 第一次 ~30-60 分鐘，之後增量幾秒
~/llvm-project/build/bin/llc -mtriple=toygpu frag.ll -o frag.s
```

## Triple patch 動了什麼（porting 的隱藏第一步）

LLVM 要認得 `-mtriple=toygpu`，得在 TargetParser 註冊 arch：
- `llvm/include/llvm/TargetParser/Triple.h`：ArchType enum 加 `toygpu`
- `llvm/lib/TargetParser/Triple.cpp`：名字對映（getArchTypeName /
  parseArch / getArchPointerBitWidth=32）

setup-llvm.sh 用帶 guard 的 sed 做這件事（idempotent，重跑不會重複插）。

## 實際踩到的坑（porting 教材，C1 階段全記錄）

1. **tblgen: "Type set is empty for each HW mode in atomic_cmp_swap_i8"**
   共用的 TargetSelectionDAG.td 有 atomic PatFrags，要求 target 至少
   存在一種整數型別。純 f32 register file 直接掛。
   → RegisterClass 掛上 `[f32, i32]`。
2. **runtime assert: "No integer registers defined!"**
   上一題的 runtime 雙胞胎：`computeRegisterProperties` 要求至少一個
   整數 register class。→ `addRegisterClass(MVT::i32, &FPRRegClass)`。
3. **segfault in mayRaiseFPException**
   Subtarget 沒提供 `SelectionDAGTargetInfo`（base 回 nullptr、
   caller 不檢查）→ 一遇到 target node 就 deref null。
   → 學 MSP430：塞一個 plain `SelectionDAGTargetInfo TSInfo` 成員。
4. **UNREACHABLE "Invalid architecture value" (getArchPointerBitWidth)**
   Triple.cpp 有兩種 case 寫法（`Triple::` 和 `llvm::Triple::` 前綴），
   兩處都要加 toygpu。setup-llvm.sh 的 sed 現在兩種都 patch。

C1 驗收：`define void @main(){ret void}` → `llc -mtriple=toygpu` → `RET` ✓

## C2 踩坑（算術 + 特殊暫存器 lowering）

5. **tblgen: def already exists: f32imm** — `f32imm` 是共用 td 的保留名
   → 改用 `tgf32imm`。
6. **Cannot select: fpimm → constant-pool load** — f32 常數預設 spill 到
   constant pool，但我們沒有 load-from-memory 指令 → override
   `isFPImmLegal` 回 true，讓 fpimm 節點留著給 LDI pattern 配。
7. **Cannot select: STORE_OUT（TargetConstant 對 imm）** — LowerCall 用
   `getTargetConstant` 建 index，pattern 要用 `timm` 而非 `imm` 承接。
8. **只吐 RET，輸出全被 DCE** — STOUT 沒 side effect，machine-level DCE
   把整條輸出鏈刪光 → STOUT 加 `let hasSideEffects = 1;`（注意：不能
   同時顯式設 `mayStore`，會和 pattern 推導衝突，tblgen 報
   "Pattern doesn't match mayStore = 1"）。

C2 驗收：frag.ll → `llc -mtriple=toygpu` → 正好是 SPEC 的期望組語，
再過 toyasm → GPU → 同一張漸層三角形（golden test ✓）。全鏈打通。
