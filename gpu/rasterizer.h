// rasterizer — edge function + barycentric 插值，對每個 covered pixel
// 跑一次 shader core。
#pragma once
#include "shader_core.h"
#include <cstdint>
#include <vector>

struct TgVertex { float x, y; float r, g, b; };   // NDC 座標 [-1,1]

// framebuffer：RGBA8，row 0 = 圖片頂端（PNG 習慣）。
// NDC y 朝上 → 寫入時做 y 翻轉（rasterizer 內處理，呼叫端不用管）。
struct Framebuffer {
    int w, h;
    std::vector<uint8_t> rgba;                    // w*h*4
    Framebuffer(int w_, int h_) : w(w_), h(h_), rgba(w_ * h_ * 4, 0) {}
    bool savePNG(const char *path) const;
};

// 對整個 framebuffer 光柵化一個三角形：
//   program  = shader binary（每 pixel 執行）
//   uniforms = 寫進 U0-U7
// 回傳 shader 執行過的 pixel 數；shader 出錯回傳 -1（立即中止）。
int rasterize(const TgVertex v[3], const std::vector<uint32_t> &program,
              const float uniforms[8], Framebuffer &fb, bool trace = false);
