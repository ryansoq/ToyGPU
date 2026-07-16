# ToyGPU ISA v1

## 執行模型

- **單 lane、scalar float32、無 SIMT、無分支。**
- Fragment shader 對每個 covered pixel 執行一次：PC 從 0 跑到 `RET`。
- Rasterizer 在執行前把插值後的 vertex attribute 寫進 input registers；
  shader 結束（`RET`）後，GPU 從 output registers 取 RGBA。

## Register file

| 名稱 | 數量 | 編碼 | 用途 |
|---|---|---|---|
| `R0`–`R15` | 16 | 0–15  | 通用（float32），shader 可讀寫 |
| `I0`–`I3`  | 4  | 16–19 | 輸入：rasterizer 寫入插值 attribute（shader 唯讀） |
| `U0`–`U7`  | 8  | 20–27 | uniform：draw 前由 driver 寫入（shader 唯讀） |
| `O0`–`O3`  | 4  | 28–31 | 輸出：shader 寫 RGBA（僅 STOUT 可寫） |

**存取規則（interpreter 會強制）**：
- ALU 指令（MOV/FADD/FSUB/FMUL/FMA）的運算元**只能是 R**。
- I/U 只能經由 `LDIN`/`LDUNI` 讀入 R；O 只能經由 `STOUT` 寫入。
- 違規（ALU 碰 I/U/O、寫唯讀區）→ 大聲報錯並中止。

## 指令編碼

固定 32-bit word，little-endian：

```
[ 7:0] opcode
[15:8] dst
[23:16] src1
[31:24] src2（不用時為 0）
```

**例外：`LDI` 佔 2 個 word** — 第一個 word 同上格式（src1/src2 保留 0），
第二個 word 是 IEEE-754 float32 immediate。

## 指令集

| 助憶碼 | Opcode | 語意 |
|---|---|---|
| `NOP`               | 0x00 | 無動作 |
| `MOV Rd, Ra`        | 0x01 | Rd = Ra |
| `LDI Rd, imm32f`    | 0x02 | Rd = float immediate（下一個 word） |
| `FADD Rd, Ra, Rb`   | 0x03 | Rd = Ra + Rb |
| `FSUB Rd, Ra, Rb`   | 0x04 | Rd = Ra − Rb |
| `FMUL Rd, Ra, Rb`   | 0x05 | Rd = Ra × Rb |
| `FMA  Rd, Ra, Rb`   | 0x06 | Rd = Ra × Rb + **Rd**（累加器 = 目的地，tied） |
| `LDIN Rd, In`       | 0x07 | Rd = I[n]（dst=R 編碼、src1=I 編碼 16–19） |
| `LDUNI Rd, Un`      | 0x08 | Rd = U[n]（src1=U 編碼 20–27） |
| `STOUT On, Ra`      | 0x09 | O[n] = Ra（dst=O 編碼 28–31、src1=R 編碼） |
| `RET`               | 0x0A | 結束本 pixel 的執行 |
| `TRAP`              | 0xFF | dump 全部暫存器到 stderr（debug 用），繼續執行 |

## 組合語言語法（toyasm 輸入）

- 一行一指令；`;` 之後是註解；空行忽略；助憶碼與暫存器大小寫不敏感。
- 範例（本專案的驗收 shader）：

```asm
; gradient triangle: pass interpolated RGB through
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

## 設計筆記（為什麼這樣設計）

- **I/U/O 分離而非統一定址**：讓 backend 的 register allocator 只需要管 R
  （I/U/O 在編譯器眼中是 LDIN/LDUNI/STOUT 的立即數 operand，不是暫存器）。
  這是真 GPU ISA 常見的 special register file 思路的極簡版。
- **FMA 的 dst=accumulator**：destructive destination 是真實 GPU/DSP ISA
  的常見設計（省一個 operand 欄位）；在 LLVM 端會變成 tied-operand
  constraint —— 故意留這個「真的坑」當教材。
- **LDI 走雙 word**：8-bit 欄位塞不下 f32；分離 immediate word 比壓縮
  編碼簡單，fetch 邏輯只多一行。
