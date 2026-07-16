// ISA interpreter：fetch → decode → execute，直到 RET。
//
// 存取規則照 isa.md 強制執行：ALU 只碰 R；I/U 只能被 LDIN/LDUNI 讀；
// O 只能被 STOUT 寫。任何違規大聲報錯、中止該 pixel（回傳 false）——
// 「出錯要大聲」是全案原則：沉默的錯誤在圖形管線裡會變成幾小時的找 bug。
#include "shader_core.h"
#include <cstdio>
#include <cstring>

namespace {

enum Op : uint8_t {
    NOP = 0x00, MOV = 0x01, LDI = 0x02, FADD = 0x03, FSUB = 0x04,
    FMUL = 0x05, FMA = 0x06, LDIN = 0x07, LDUNI = 0x08, STOUT = 0x09,
    RET = 0x0A, TRAP = 0xFF,
};

bool isR(uint8_t r) { return r < 16; }
bool isI(uint8_t r) { return r >= 16 && r < 20; }
bool isU(uint8_t r) { return r >= 20 && r < 28; }
bool isO(uint8_t r) { return r >= 28 && r < 32; }

bool fail(size_t pc, const char *msg, uint8_t reg) {
    fprintf(stderr, "shader_core: pc=%zu: %s (reg encoding %u)\n", pc, msg, reg);
    return false;
}

void dumpState(const GpuState &st) {
    fprintf(stderr, "  R:");
    for (int i = 0; i < 16; i++) fprintf(stderr, " %g", st.R[i]);
    fprintf(stderr, "\n  I: %g %g %g %g\n", st.I[0], st.I[1], st.I[2], st.I[3]);
    fprintf(stderr, "  U:");
    for (int i = 0; i < 8; i++) fprintf(stderr, " %g", st.U[i]);
    fprintf(stderr, "\n  O: %g %g %g %g\n", st.O[0], st.O[1], st.O[2], st.O[3]);
}

} // namespace

bool shader_run(const std::vector<uint32_t> &program, GpuState &st, bool trace) {
    size_t pc = 0;
    while (pc < program.size()) {
        const uint32_t w = program[pc];
        const uint8_t op   = w & 0xFF;
        const uint8_t dst  = (w >> 8) & 0xFF;
        const uint8_t src1 = (w >> 16) & 0xFF;
        const uint8_t src2 = (w >> 24) & 0xFF;

        if (trace)
            fprintf(stderr, "[trace] pc=%zu op=0x%02x dst=%u src1=%u src2=%u\n",
                    pc, op, dst, src1, src2);

        switch (op) {
        case NOP: break;
        case RET: return true;
        case TRAP: dumpState(st); break;

        case LDI: {                        // 雙 word：下一個 word 是 f32
            if (!isR(dst)) return fail(pc, "LDI dst must be R", dst);
            if (pc + 1 >= program.size()) return fail(pc, "LDI missing imm word", dst);
            float imm;
            uint32_t iw = program[pc + 1];
            memcpy(&imm, &iw, 4);
            st.R[dst] = imm;
            pc += 2;
            continue;                      // 已自行推進 pc
        }

        case MOV:
            if (!isR(dst) || !isR(src1)) return fail(pc, "MOV operands must be R", dst);
            st.R[dst] = st.R[src1];
            break;

        case FADD: case FSUB: case FMUL: case FMA: {
            if (!isR(dst) || !isR(src1) || !isR(src2))
                return fail(pc, "ALU operands must be R", dst);
            const float a = st.R[src1], b = st.R[src2];
            if (op == FADD) st.R[dst] = a + b;
            if (op == FSUB) st.R[dst] = a - b;
            if (op == FMUL) st.R[dst] = a * b;
            if (op == FMA)  st.R[dst] = a * b + st.R[dst];   // dst 即累加器（tied）
            break;
        }

        case LDIN:
            if (!isR(dst))  return fail(pc, "LDIN dst must be R", dst);
            if (!isI(src1)) return fail(pc, "LDIN src must be I0-I3", src1);
            st.R[dst] = st.I[src1 - 16];
            break;

        case LDUNI:
            if (!isR(dst))  return fail(pc, "LDUNI dst must be R", dst);
            if (!isU(src1)) return fail(pc, "LDUNI src must be U0-U7", src1);
            st.R[dst] = st.U[src1 - 20];
            break;

        case STOUT:
            if (!isO(dst))  return fail(pc, "STOUT dst must be O0-O3", dst);
            if (!isR(src1)) return fail(pc, "STOUT src must be R", src1);
            st.O[dst - 28] = st.R[src1];
            break;

        default:
            return fail(pc, "unknown opcode", op);
        }
        pc += 1;
    }
    fprintf(stderr, "shader_core: fell off end of program without RET\n");
    return false;
}
