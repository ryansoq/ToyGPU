// toygl 實作：全域單一 context（v1 夠用）。
// tgCompileShader 依副檔名分流：
//   .bin       → 直接載入
//   .s         → exec toyasm 轉 bin（中間檔留在 build/）
//   .glsl      → （階段 4）glslangValidator → spirv2llvm → llc → toyasm
#include "toygl.h"
#include "../gpu/rasterizer.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
Framebuffer *g_fb = nullptr;
std::vector<uint32_t> g_shader;
float g_uniforms[8] = {};

std::vector<uint32_t> loadBin(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) { fprintf(stderr, "toygl: cannot open %s\n", path.c_str()); return {}; }
    std::vector<uint32_t> words;
    uint32_t w;
    while (f.read(reinterpret_cast<char *>(&w), 4)) words.push_back(w);
    return words;
}

bool endsWith(const std::string &s, const char *suf) {
    size_t n = strlen(suf);
    return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
}
} // namespace

void tgInit(int w, int h) {
    delete g_fb;
    g_fb = new Framebuffer(w, h);
}

int tgCompileShader(const char *path) {
    std::string p(path);
    if (system("mkdir -p build") != 0) return -1;
    if (endsWith(p, ".bin")) {
        g_shader = loadBin(p);
    } else if (endsWith(p, ".s")) {
        std::string cmd = "python3 toyasm/toyasm.py " + p + " build/shader.bin";
        if (system(cmd.c_str()) != 0) return -1;
        g_shader = loadBin("build/shader.bin");
    } else if (endsWith(p, ".glsl")) {
        // 完整編譯鏈：glslang → spirv2llvm → llc → toyasm。
        // llc 路徑用環境變數 TOYGPU_LLC 覆蓋（預設用我們 build 的 llc）。
        const char *llcEnv = getenv("TOYGPU_LLC");
        std::string LLC = llcEnv ? llcEnv : "../llvm-project/build/bin/llc";
        std::string b = "build/shader";
        std::string cmd =
            "glslangValidator -V -S frag " + p + " -o " + b + ".spv && "
            "build/spirv2llvm " + b + ".spv " + b + ".ll && " +
            LLC + " -mtriple=toygpu " + b + ".ll -o " + b + ".s.raw && "
            // 濾掉 llc 的 directive/label，只留指令給 toyasm
            "grep -E '^[[:space:]]+(LDIN|LDUNI|STOUT|LDI|FADD|FSUB|FMUL|FMA|MOV|RET|NOP)' "
            + b + ".s.raw | sed 's/e+00//' > " + b + ".s && "
            "python3 toyasm/toyasm.py " + b + ".s " + b + ".bin";
        if (system(cmd.c_str()) != 0) {
            fprintf(stderr, "toygl: GLSL compile chain failed\n");
            return -1;
        }
        g_shader = loadBin(b + ".bin");
    } else {
        fprintf(stderr, "toygl: unsupported shader type: %s\n", path);
        return -1;
    }
    return g_shader.empty() ? -1 : 0;
}

void tgBindShader(int) { /* v1 單 slot，載入即綁定 */ }
void tgSetUniform(int i, float v) { if (i >= 0 && i < 8) g_uniforms[i] = v; }

int tgDrawTriangle(const TgVertex2 v[3]) {
    if (!g_fb || g_shader.empty()) { fprintf(stderr, "toygl: not initialized\n"); return -1; }
    TgVertex tv[3];
    for (int i = 0; i < 3; i++) tv[i] = {v[i].x, v[i].y, v[i].r, v[i].g, v[i].b};
    return rasterize(tv, g_shader, g_uniforms, *g_fb);
}

bool tgSavePNG(const char *path) { return g_fb && g_fb->savePNG(path); }
