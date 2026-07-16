#!/usr/bin/env python3
"""toyasm — ToyGPU 組譯器：.s 文字 → raw binary（little-endian u32 流）

用法：python3 toyasm.py input.s output.bin

設計：一行一指令、無 label（ISA v1 無分支；語法上把「操作數解析」
獨立成函式，之後加分支/label 只需擴充那裡）。錯誤一律帶行號大聲報。
"""

import struct
import sys

# opcode 表（與 isa.md 一一對應）
OPCODES = {
    "NOP":   0x00, "MOV":  0x01, "LDI":   0x02, "FADD": 0x03,
    "FSUB":  0x04, "FMUL": 0x05, "FMA":   0x06, "LDIN": 0x07,
    "LDUNI": 0x08, "STOUT": 0x09, "RET":  0x0A, "TRAP": 0xFF,
}

# 每條指令的 operand 形態：R=通用暫存器 I/U/O=特殊暫存器 F=float immediate
SIGNATURES = {
    "NOP":  [],          "RET": [],           "TRAP": [],
    "MOV":  ["R", "R"],  "LDI": ["R", "F"],
    "FADD": ["R", "R", "R"], "FSUB": ["R", "R", "R"],
    "FMUL": ["R", "R", "R"], "FMA":  ["R", "R", "R"],
    "LDIN": ["R", "I"],  "LDUNI": ["R", "U"], "STOUT": ["O", "R"],
}

# 暫存器編碼：R0-15=0-15, I0-3=16-19, U0-7=20-27, O0-3=28-31
REG_BASE = {"R": (0, 16), "I": (16, 4), "U": (20, 8), "O": (28, 4)}


def parse_reg(tok, want, lineno):
    """把 'R3' / 'I0' 之類解析成 5-bit 編碼，並檢查型別與範圍。"""
    tok = tok.upper()
    kind, num = tok[0], tok[1:]
    if kind != want:
        die(lineno, f"expected {want}-register, got '{tok}'")
    if not num.isdigit():
        die(lineno, f"bad register '{tok}'")
    base, count = REG_BASE[kind]
    n = int(num)
    if n >= count:
        die(lineno, f"register '{tok}' out of range (max {want}{count-1})")
    return base + n


def die(lineno, msg):
    sys.stderr.write(f"toyasm: line {lineno}: {msg}\n")
    sys.exit(1)


def assemble(text):
    words = []
    for lineno, raw in enumerate(text.splitlines(), 1):
        line = raw.split(";")[0].strip()        # 去註解、去空白
        if not line:
            continue
        parts = line.replace(",", " ").split()
        mn = parts[0].upper()
        if mn not in OPCODES:
            die(lineno, f"unknown mnemonic '{parts[0]}'")
        sig, ops = SIGNATURES[mn], parts[1:]
        if len(ops) != len(sig):
            die(lineno, f"{mn} expects {len(sig)} operands, got {len(ops)}")

        fields = [OPCODES[mn], 0, 0, 0]          # [opcode, dst, src1, src2]
        imm = None
        for i, (want, tok) in enumerate(zip(sig, ops)):
            if want == "F":
                try:
                    imm = float(tok)
                except ValueError:
                    die(lineno, f"bad float immediate '{tok}'")
            else:
                fields[1 + i] = parse_reg(tok, want, lineno)

        words.append(fields[0] | (fields[1] << 8) | (fields[2] << 16) | (fields[3] << 24))
        if imm is not None:                       # LDI 的第二個 word
            words.append(struct.unpack("<I", struct.pack("<f", imm))[0])
    return b"".join(struct.pack("<I", w) for w in words)


def main():
    if len(sys.argv) != 3:
        sys.stderr.write("usage: toyasm.py input.s output.bin\n")
        sys.exit(2)
    with open(sys.argv[1]) as f:
        binary = assemble(f.read())
    with open(sys.argv[2], "wb") as f:
        f.write(binary)
    print(f"toyasm: {len(binary)//4} words -> {sys.argv[2]}")


if __name__ == "__main__":
    main()
