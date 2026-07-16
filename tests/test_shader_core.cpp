// interpreter 單元測試：手刻 bytecode，不依賴 toyasm（兩工具獨立可測）。
#include "../gpu/shader_core.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static uint32_t enc(uint8_t op, uint8_t d = 0, uint8_t s1 = 0, uint8_t s2 = 0) {
    return op | (d << 8) | (s1 << 16) | (s2 << 24);
}
static uint32_t f2w(float f) { uint32_t w; memcpy(&w, &f, 4); return w; }

int main() {
    // 1. gradient 流程：I → R → O
    {
        GpuState st; st.I[0] = 0.25f; st.I[1] = 0.5f; st.I[2] = 0.75f;
        std::vector<uint32_t> prog = {
            enc(0x07, 0, 16), enc(0x07, 1, 17), enc(0x07, 2, 18),   // LDIN
            enc(0x02, 3), f2w(1.0f),                                // LDI R3,1.0
            enc(0x09, 28, 0), enc(0x09, 29, 1), enc(0x09, 30, 2), enc(0x09, 31, 3),
            enc(0x0A),
        };
        assert(shader_run(prog, st));
        assert(st.O[0] == 0.25f && st.O[1] == 0.5f && st.O[2] == 0.75f && st.O[3] == 1.0f);
    }
    // 2. FMA tied dst：R2 = R0*R1 + R2
    {
        GpuState st;
        std::vector<uint32_t> prog = {
            enc(0x02, 0), f2w(2.0f), enc(0x02, 1), f2w(3.0f), enc(0x02, 2), f2w(10.0f),
            enc(0x06, 2, 0, 1),                                     // FMA R2,R0,R1
            enc(0x0A),
        };
        assert(shader_run(prog, st));
        assert(st.R[2] == 16.0f);
    }
    // 3. 違規要大聲：ALU 碰 I 暫存器 → false
    {
        GpuState st;
        std::vector<uint32_t> prog = { enc(0x03, 0, 16, 1), enc(0x0A) };  // FADD R0,I0,R1
        assert(!shader_run(prog, st));
    }
    // 4. 沒 RET 掉出結尾 → false
    {
        GpuState st;
        std::vector<uint32_t> prog = { enc(0x00) };
        assert(!shader_run(prog, st));
    }
    printf("test_shader_core: all pass\n");
    return 0;
}
