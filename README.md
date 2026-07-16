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

- [ ] 階段 1：軟體 GPU + toyasm + toygl（手寫 .s 畫出漸層三角形）
- [ ] 階段 2：LLVM ToyGPU backend（llc 吐 .s）
- [ ] 階段 3：spirv2llvm（SPIR-V walker → LLVM IR）
- [ ] 階段 4：pipeline 串接（GLSL 一鍵出圖）

## 快速開始（階段 1）

```bash
make            # 編譯 gpu + demo
make test       # 跑 golden test
./triangle      # 產出 triangle.png（512×512 漸層三角形）
```

詳細規格見 [SPEC.md](SPEC.md)，ISA 定義見 [isa.md](isa.md)。

## 學習筆記

每個 commit 是一課（單一主題、可獨立編譯、訊息解釋為什麼）。
`git log --oneline` 就是課程目錄。
