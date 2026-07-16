# ToyGPU — 軟體 GPU + LLVM Backend 完整編譯流程專案規格

## 專案目標

打造一條**乾淨、流暢、最小化**的 GPU shader 編譯與執行流程,最終畫出一張漸層三角形 PNG:

```
GLSL fragment shader
   │  glslangValidator(外部工具,離線呼叫)
   ▼
SPIR-V (.spv)
   │  spirv2llvm(自寫工具,~500 行)
   ▼
LLVM IR (.ll)
   │  llc -march=toygpu(自寫 LLVM backend)★ 核心 1
   ▼
ToyGPU 組合語言 (.s)
   │  toyasm(自寫 Python assembler,~50-100 行)
   ▼
ToyGPU ISA binary (.bin)
   │
   ▼
軟體 GPU(rasterizer + ISA interpreter)★ 核心 2
   │
   ▼
triangle.png(漸層三角形)
```

重點放在 **LLVM backend** 和**軟體 GPU** 兩塊,其他部分越薄越好。

## 使用者程式(最終長相)

`main.cpp` 使用極簡 API(不是 OpenGL,只有 5 個函數):

```cpp
#include "toygl.h"

int main() {
    tgInit(512, 512);

    // 內部跑完整條編譯鏈:GLSL → SPIR-V → LLVM IR → asm → binary
    TgShader fs = tgCompileShader("shaders/frag.glsl");
    tgBindShader(fs);

    TgVertex v[3] = {
        //  x      y     r  g  b
        { -0.8f, -0.8f,  1, 0, 0 },
        {  0.8f, -0.8f,  0, 1, 0 },
        {  0.0f,  0.8f,  0, 0, 1 },
    };
    tgDrawTriangle(v);          // rasterize + per-pixel 執行 shader core

    tgSavePNG("triangle.png");
}
```

`tgCompileShader()` 內部用 `exec` 依序呼叫外部工具(glslangValidator → spirv2llvm → llc → toyasm),中間檔全部保留在 `/tmp` 或 `build/` 方便 debug。main 程式**不 link LLVM**。

## Repo 結構

```
toygpu/
├── README.md
├── isa.md                  # ISA 規格書
├── main.cpp                # 上面的 demo 程式
├── toygl/
│   ├── toygl.h              # tgInit/tgCompileShader/tgBindShader/tgDrawTriangle/tgSavePNG
│   ├── toygl.cpp
│   └── pipeline.cpp         # exec 編譯工具鏈 + 載入 binary
├── gpu/                     # 軟體 GPU,純 C++,零外部依賴(PNG 用 stb_image_write)
│   ├── shader_core.h/.cpp   # ISA interpreter
│   ├── rasterizer.h/.cpp    # edge function + barycentric 插值
│   └── framebuffer.h/.cpp   # RGBA8 buffer + 輸出 PNG
├── spirv2llvm/              # 工具:SPIR-V → LLVM IR
│   └── main.cpp             # 用 spirv-headers 解析,LLVM IRBuilder 輸出
├── backend/                 # LLVM ToyGPU target(out-of-tree 或 patch llvm-project)
│   ├── ToyGPU.td
│   ├── ToyGPURegisterInfo.td
│   ├── ToyGPUInstrInfo.td
│   ├── ToyGPUISelLowering.cpp
│   ├── ToyGPUAsmPrinter.cpp
│   └── ...
├── toyasm/
│   └── toyasm.py            # 文字組語 → binary
├── shaders/
│   └── frag.glsl            # 固定的漸層 fragment shader
└── tests/
    ├── handwritten.bin 相關測試
    └── FileCheck 測試(.ll → .s)
```

## ISA 設計(isa.md 的內容)

### 執行模型

- 單 lane、scalar float、無 SIMT、無分支(第一版)。
- Fragment shader 對每個 covered pixel 執行一次,從 PC=0 跑到 `RET`。
- Rasterizer 負責把插值後的 vertex attribute 填入 input registers,shader 結束後從 output registers 取出顏色。

### Register file

| 名稱 | 數量 | 用途 |
|---|---|---|
| `R0`–`R15` | 16 | 通用暫存器(float32) |
| `I0`–`I3`  | 4  | 輸入(rasterizer 寫入插值後的 attribute;唯讀) |
| `U0`–`U7`  | 8  | uniform(driver 在 draw 前寫入;唯讀) |
| `O0`–`O3`  | 4  | 輸出(shader 寫入 RGBA;RET 後由 GPU 讀取) |

### 指令集(~10 條)

固定 32-bit 指令長度,格式:`[8-bit opcode][8-bit dst][8-bit src1][8-bit src2]`。
LDI 例外:`[8-bit opcode][8-bit dst][16-bit 保留]` + 後面跟一個 32-bit float immediate(即 LDI 佔 2 個 word)。

| 助憶碼 | Opcode | 語意 |
|---|---|---|
| `NOP`  | 0x00 | 無動作 |
| `MOV Rd, Ra`        | 0x01 | Rd = Ra |
| `LDI Rd, imm32f`    | 0x02 | Rd = float immediate(下一個 word) |
| `FADD Rd, Ra, Rb`   | 0x03 | Rd = Ra + Rb |
| `FSUB Rd, Ra, Rb`   | 0x04 | Rd = Ra - Rb |
| `FMUL Rd, Ra, Rb`   | 0x05 | Rd = Ra * Rb |
| `FMA  Rd, Ra, Rb`   | 0x06 | Rd = Ra * Rb + Rd |
| `LDIN Rd, In`       | 0x07 | Rd = I[n](讀插值輸入) |
| `LDUNI Rd, Un`      | 0x08 | Rd = U[n](讀 uniform) |
| `STOUT On, Ra`      | 0x09 | O[n] = Ra(寫輸出) |
| `RET`               | 0x0A | 結束 |
| `TRAP`              | 0xFF | dump 全部暫存器到 stderr(debug 用) |

register 編碼:R0–R15 = 0–15,I0–I3 = 16–19,U0–U7 = 20–27,O0–O3 = 28–31。

### 組合語言語法(toyasm 輸入)

```
; 漸層三角形:直接把插值後的 RGB 輸出
LDIN  R0, I0      ; r
LDIN  R1, I1      ; g
LDIN  R2, I2      ; b
LDI   R3, 1.0     ; alpha
STOUT O0, R0
STOUT O1, R1
STOUT O2, R2
STOUT O3, R3
RET
```

## 軟體 GPU 規格(gpu/)

### Rasterizer

- 輸入:3 個 `TgVertex { float x, y; float r, g, b; }`,NDC 座標([-1,1])。
- Viewport transform 到像素座標。
- Edge function 判斷 pixel 中心是否在三角形內(top-left rule 可簡化,不強求)。
- Barycentric 權重 (w0, w1, w2) 對 r/g/b 各自插值 → 填入 `I0, I1, I2`。
- 對每個 covered pixel 呼叫 shader core,取 `O0..O3` 轉 RGBA8 寫入 framebuffer。

### Shader core(interpreter)

- `struct GpuState { float R[16], I[4], U[8], O[4]; };`
- fetch-decode-execute 迴圈直到 `RET`。
- `TRAP` 印出所有暫存器。
- 對未知 opcode 報錯並中止(不要沉默跳過)。

### Framebuffer

- RGBA8,y 軸向上(或明確註明方向)。
- 用 stb_image_write 輸出 PNG。

## Fragment shader(shaders/frag.glsl)

```glsl
#version 450
layout(location = 0) in vec3 vColor;
layout(location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
```

編譯:`glslangValidator -V frag.glsl -o frag.spv`

## spirv2llvm 規格

自寫一個小的 SPIR-V walker(不要用 SPIRV-LLVM-Translator,它對 graphics shader 支援不好):

- 用 spirv-headers(僅標頭檔)解析 binary。
- 需支援的 opcodes(以上面 shader 的輸出為準,實際跑 `spirv-dis` 確認):
  `OpCapability, OpMemoryModel, OpEntryPoint, OpDecorate(Location), OpTypeFloat, OpTypeVector, OpTypePointer, OpTypeVoid, OpTypeFunction, OpVariable, OpConstant, OpConstantComposite, OpLoad, OpStore, OpCompositeExtract, OpCompositeConstruct, OpFunction, OpLabel, OpReturn, OpFunctionEnd`
- 也建議支援 `OpFAdd/OpFSub/OpFMul` 以便之後 shader 變複雜。
- 輸出的 LLVM IR 介面約定(這是 spirv2llvm 與 backend 之間的 ABI):
  - Vector 全部 scalarize 成 float(vec3 input → 3 個 scalar)。
  - 讀 input:`declare float @toygpu.input(i32)`(intrinsic-style 外部函數,參數 = flat 化後的 channel index;`Location 0` 的 vec3 → index 0,1,2)。
  - 讀 uniform:`declare float @toygpu.uniform(i32)`。
  - 寫 output:`declare void @toygpu.output(i32, float)`。
  - main 函數:`define void @main()`,結尾 `ret void`。
- 上面 shader 的期望輸出示意:

```llvm
declare float @toygpu.input(i32)
declare void @toygpu.output(i32, float)

define void @main() {
entry:
  %r = call float @toygpu.input(i32 0)
  %g = call float @toygpu.input(i32 1)
  %b = call float @toygpu.input(i32 2)
  call void @toygpu.output(i32 0, float %r)
  call void @toygpu.output(i32 1, float %g)
  call void @toygpu.output(i32 2, float %b)
  call void @toygpu.output(i32 3, float 1.0)
  ret void
}
```

## LLVM backend 規格(backend/)

- Target 名稱 `toygpu`,SelectionDAG 路線(不用 GlobalISel)。
- 參考 `llvm/lib/Target/Sparc`(最小的 in-tree target 之一)或 Cpu0 教學的骨架。
- 需要的元件:
  - `ToyGPURegisterInfo.td`:R0–R15 為可分配的 FPR class;I/U/O 為 reserved 特殊暫存器(或干脆不建模,見下)。
  - `ToyGPUInstrInfo.td`:上面 ISA 的指令 + patterns(`fadd/fsub/fmul/fma → FADD/FSUB/FMUL/FMA`,`f32 immediate → LDI`)。
  - `ToyGPUISelLowering.cpp`:把 `@toygpu.input/@toygpu.uniform/@toygpu.output` 這三個 call lower 成 `LDIN/LDUNI/STOUT`(建議用 custom lowering 把 call 換成 target-specific SDNode,再用 pattern match 到指令;index 參數必須是常數)。
  - `ToyGPUAsmPrinter`:輸出上面定義的文字組語格式。
  - Calling convention 幾乎是空的:main 無參數無回傳,不需要 stack(第一版 spill 可以直接報錯或保留極簡 stack)。
- Register allocation 用預設 Greedy 即可,16 個 R 對這個 shader 綽綽有餘。
- **第一版不做 MCCodeEmitter**,`llc` 吐 `.s` 文字,由 toyasm 轉 binary。之後可作為 stretch goal 補直接 object emission。
- Out-of-tree build 或 fork llvm-project 加 target 皆可,取容易者(建議 fork + `LLVM_EXPERIMENTAL_TARGETS_TO_BUILD=ToyGPU`)。

## toyasm 規格

- Python,讀 `.s` 文字(上面語法),輸出 raw binary(little-endian u32 序列)。
- 支援 `;` 註解、空行、大小寫不敏感。
- 錯誤要報行號。

## 開發階段(嚴格照順序,每階段都有可驗證產出)

### 階段 1:GPU 先行(不碰 LLVM)
- 寫 `isa.md`、`gpu/`、`toyasm/`、`toygl/`、`main.cpp`。
- `tgCompileShader()` 暫時直接載入手寫的 `.s`(經 toyasm)或 hardcoded bytecode。
- **驗收:跑 `./triangle` 產出 512×512 漸層三角形 PNG(三個角分別是紅/綠/藍,中間平滑漸層)。**

### 階段 2:LLVM backend
- 建 ToyGPU target。
- 手寫上面「期望輸出示意」那段 `.ll`,`llc -march=toygpu` 出 `.s`。
- **驗收:llc 輸出的 .s 經 toyasm → binary,餵進階段 1 的 GPU,畫出同一張三角形。加 2–3 個 FileCheck 測試(fadd/fmul/常數)。**

### 階段 3:spirv2llvm
- `glslangValidator -V frag.glsl -o frag.spv`,先用 `spirv-dis` 看實際指令,照著實作 walker。
- **驗收:frag.spv → .ll → llc → .s → .bin → GPU → 同一張三角形。**

### 階段 4:串接
- `pipeline.cpp` 用 exec 把四個工具串起來,`tgCompileShader("frag.glsl")` 一路到 binary。
- **驗收:`main.cpp` 從 GLSL 原始碼開始,一鍵出圖。**

### Stretch goals(選做)
- MCCodeEmitter 直接吐 binary(移除 toyasm 依賴)。
- shader 加運算(如 `fragColor = vec4(vColor * uScale, 1.0)`)驗證 FMUL/LDUNI 路徑。
- 簡單分支(BEQ/BLT + OpBranchConditional)。
- Depth test、多三角形。

## 全域原則

- 每個工具獨立可執行、獨立可測;中間產物(.spv/.ll/.s/.bin)全部落地保留。
- 出錯要大聲:未知 opcode、超界 register、非常數 index 一律報錯中止。
- 程式碼精簡優先,不做用不到的抽象。gpu/ 保持零依賴(除 stb_image_write)。
- 註解用途:解釋「為什麼」,不是「做什麼」。
