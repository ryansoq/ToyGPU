// shader_core — ToyGPU 的心臟：ISA interpreter
//
// 一個 GpuState = 一次 pixel 的執行環境。rasterizer 填 I[]，
// run() 從 PC=0 跑到 RET，呼叫端再從 O[] 取顏色。
#pragma once
#include <cstdint>
#include <vector>

struct GpuState {
    float R[16] = {};   // 通用
    float I[4]  = {};   // 輸入（rasterizer 寫）
    float U[8]  = {};   // uniform（driver 寫）
    float O[4]  = {};   // 輸出（shader 寫、GPU 讀）
};

// program = little-endian u32 word 流（toyasm 的輸出）。
// trace = true 時逐指令 dump（debug/教學用）。
// 回傳 false 表示執行錯誤（未知 opcode、暫存器違規 …），錯誤印到 stderr。
bool shader_run(const std::vector<uint32_t> &program, GpuState &st, bool trace = false);
