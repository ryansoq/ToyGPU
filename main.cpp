// demo：畫一張 512×512 漸層三角形（紅/綠/藍三個角）。
#include "toygl/toygl.h"
#include <cstdio>

int main() {
    tgInit(512, 512);

    if (tgCompileShader("shaders/gradient.s") != 0) return 1;
    tgBindShader(0);

    TgVertex2 v[3] = {
        //   x      y     r  g  b
        { -0.8f, -0.8f,  1, 0, 0 },     // 左下：紅
        {  0.8f, -0.8f,  0, 1, 0 },     // 右下：綠
        {  0.0f,  0.8f,  0, 0, 1 },     // 頂：藍
    };
    int n = tgDrawTriangle(v);
    if (n < 0) return 1;
    printf("shaded %d pixels\n", n);

    if (!tgSavePNG("triangle.png")) return 1;
    printf("wrote triangle.png\n");
    return 0;
}
