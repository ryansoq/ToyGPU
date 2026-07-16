// toygl — 極簡 5 函數 API（不是 OpenGL，只是長得有點像）
#pragma once

struct TgVertex2 { float x, y, r, g, b; };

void tgInit(int w, int h);
// 階段 1：吃 .s（經 toyasm）或 .bin；階段 4 之後 .glsl 走完整編譯鏈。
// 回傳 shader handle（v1 只有一個 slot，回傳 0；失敗回 -1）。
int  tgCompileShader(const char *path);
void tgBindShader(int handle);
void tgSetUniform(int index, float value);          // U0-U7
int  tgDrawTriangle(const TgVertex2 v[3]);          // 回傳 shaded pixel 數
bool tgSavePNG(const char *path);
