# ToyGPU ⚔️ — 軟體 GPU + 真 LLVM Backend 的最小完整編譯流程

> 一個三角形，看懂 shader 從原始碼到像素的每一個字節。

```
frag.glsl ──glslangValidator──▶ frag.spv ──spirv2llvm──▶ frag.ll
                                                            │
                    llc -mtriple=toygpu（自寫 LLVM target）★
                                                            ▼
triangle.png ◀──軟體GPU(rasterizer+interpreter)★◀── frag.bin ◀──toyasm── frag.s
```

每一站的中間產物都落地成檔案，可以單獨檢查、單獨重跑。

## 階段進度

- [x] 階段 1：軟體 GPU + toyasm + toygl（手寫 .s 畫出漸層三角形）✅
- [x] 階段 2：LLVM ToyGPU backend（真 target porting，llc 吐 .s）✅
- [x] 階段 3：spirv2llvm（SPIR-V walker → LLVM IR）✅
- [x] 階段 4：pipeline 串接（GLSL 一鍵出圖）✅

**全部完成** — `./triangle` 從 `frag.glsl` 一路到 `triangle.png`，
中間 `.spv/.ll/.s/.bin` 全部落在 `build/` 可單獨檢查。

## 快速開始

```bash
# 先建 LLVM backend（第一次 ~30-60 分鐘）
bash backend/setup-llvm.sh && ninja -C ~/llvm-project/build llc

# 然後一鍵從 GLSL 出圖
make            # 編譯 gpu + spirv2llvm + demo
./triangle      # frag.glsl → glslang → spirv2llvm → llc → toyasm → GPU → PNG
make test       # golden pixel test

# 逐站檢查中間產物
ls build/       # shader.spv / shader.ll / shader.s / shader.bin
spirv-dis build/shader.spv       # 看 SPIR-V
cat build/shader.ll              # 看 LLVM IR
cat build/shader.s               # 看 ToyGPU 組語
```

詳細規格見 [SPEC.md](SPEC.md)，ISA 定義見 [isa.md](isa.md)。

## 學習筆記

每個 commit 是一課（單一主題、可獨立編譯、訊息解釋為什麼）。
`git log --oneline` 就是課程目錄。
