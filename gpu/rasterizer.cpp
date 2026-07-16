// Edge function 光柵化（Pineda 法）——現代 GPU 光柵器的同款數學。
//
// edge(A,B,P) = (B-A) × (P-A)（2D 外積）：P 在 AB 左側為正。
// 三條邊同號 → P 在三角形內；三個 edge 值 ÷ 總面積 = barycentric 權重，
// 直接拿來對 vertex attribute 插值。
//
// 兩個經典 off-by-one 在這裡處理：
//   1. 繞向：頂點順/逆時針會讓面積變號 → 面積為負就整組翻正。
//   2. 取樣點：pixel 中心 (x+0.5, y+0.5)，不是角落。
#include "rasterizer.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <algorithm>
#include <cstdio>

bool Framebuffer::savePNG(const char *path) const {
    // rgba 的 row 0 已是圖片頂端，直接寫。
    return stbi_write_png(path, w, h, 4, rgba.data(), w * 4) != 0;
}

static float edgeFn(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

int rasterize(const TgVertex v[3], const std::vector<uint32_t> &program,
              const float uniforms[8], Framebuffer &fb, bool trace) {
    // NDC [-1,1] → 像素座標。NDC y 朝上、圖片 row 朝下 → y 翻轉。
    auto toPx = [&](const TgVertex &t, float &px, float &py) {
        px = (t.x * 0.5f + 0.5f) * fb.w;
        py = (1.0f - (t.y * 0.5f + 0.5f)) * fb.h;
    };
    float x0, y0, x1, y1, x2, y2;
    toPx(v[0], x0, y0); toPx(v[1], x1, y1); toPx(v[2], x2, y2);

    float area = edgeFn(x0, y0, x1, y1, x2, y2);
    if (area == 0) return 0;                       // 退化三角形
    // 繞向歸一：面積為負（順時針）→ 交換頂點 1/2（座標和 attribute 一起換）。
    const TgVertex *A = &v[0], *B = &v[1], *C = &v[2];
    if (area < 0) {
        std::swap(x1, x2); std::swap(y1, y2);
        std::swap(B, C);
        area = -area;
    }

    // bounding box（clamp 到畫布）
    int minx = std::max(0, (int)std::floor(std::min({x0, x1, x2})));
    int maxx = std::min(fb.w - 1, (int)std::ceil(std::max({x0, x1, x2})));
    int miny = std::max(0, (int)std::floor(std::min({y0, y1, y2})));
    int maxy = std::min(fb.h - 1, (int)std::ceil(std::max({y0, y1, y2})));

    int shaded = 0;
    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            const float px = x + 0.5f, py = y + 0.5f;      // pixel 中心取樣
            const float w0 = edgeFn(x1, y1, x2, y2, px, py);   // 對頂點 A 的權重
            const float w1 = edgeFn(x2, y2, x0, y0, px, py);   // 對 B
            const float w2 = edgeFn(x0, y0, x1, y1, px, py);   // 對 C
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;          // 在外面

            const float b0 = w0 / area, b1 = w1 / area, b2 = w2 / area;

            GpuState st;
            st.I[0] = b0 * A->r + b1 * B->r + b2 * C->r;
            st.I[1] = b0 * A->g + b1 * B->g + b2 * C->g;
            st.I[2] = b0 * A->b + b1 * B->b + b2 * C->b;
            st.I[3] = 0.0f;
            for (int i = 0; i < 8; i++) st.U[i] = uniforms[i];

            // 只 trace 第一個 pixel，不然 26 萬個 pixel 的 dump 沒人看得完
            if (!shader_run(program, st, trace && shaded == 0)) {
                fprintf(stderr, "rasterize: shader failed at pixel (%d,%d)\n", x, y);
                return -1;
            }

            auto to8 = [](float f) {
                return (uint8_t)std::clamp((int)(f * 255.0f + 0.5f), 0, 255);
            };
            uint8_t *p = &fb.rgba[(y * fb.w + x) * 4];
            p[0] = to8(st.O[0]); p[1] = to8(st.O[1]);
            p[2] = to8(st.O[2]); p[3] = to8(st.O[3]);
            shaded++;
        }
    }
    return shaded;
}
