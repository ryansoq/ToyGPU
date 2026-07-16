#!/usr/bin/env python3
"""Golden test：取樣三個角附近 + 質心，驗證漸層三角形的顏色正確。
不比對整張圖（抗實作細節差異），比對「語意上必然成立」的點。"""
import struct, sys, zlib

def read_png_rgba(path):
    data = open(path, "rb").read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", "not a PNG"
    pos, w, h, idat = 8, 0, 0, b""
    while pos < len(data):
        ln = int.from_bytes(data[pos:pos+4], "big"); typ = data[pos+4:pos+8]
        chunk = data[pos+8:pos+8+ln]
        if typ == b"IHDR":
            w, h = struct.unpack(">II", chunk[:8])
            assert chunk[8] == 8 and chunk[9] == 6, "expect 8-bit RGBA"
        elif typ == b"IDAT":
            idat += chunk
        pos += 12 + ln
    raw = zlib.decompress(idat)
    px = {}
    stride = w * 4 + 1
    prev = bytearray(w * 4)
    out_rows = []
    for y in range(h):
        row = bytearray(raw[y*stride+1:(y+1)*stride])
        ft = raw[y*stride]
        # 還原 PNG filter（stb 會用 0/1/2/3/4 都可能）
        for x in range(len(row)):
            a = row[x-4] if x >= 4 else 0
            b = prev[x]
            c = prev[x-4] if x >= 4 else 0
            if ft == 1: row[x] = (row[x] + a) & 255
            elif ft == 2: row[x] = (row[x] + b) & 255
            elif ft == 3: row[x] = (row[x] + (a + b)//2) & 255
            elif ft == 4:
                p = a + b - c
                pa, pb, pc = abs(p-a), abs(p-b), abs(p-c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                row[x] = (row[x] + pr) & 255
        prev = row
        out_rows.append(bytes(row))
    def get(x, y):
        r = out_rows[y]
        return tuple(r[x*4:x*4+4])
    return w, h, get

def near(px, want, tol=30):
    return all(abs(a-b) <= tol for a, b in zip(px[:3], want)) and px[3] == 255

w, h, get = read_png_rgba(sys.argv[1] if len(sys.argv) > 1 else "triangle.png")
assert (w, h) == (512, 512), f"size {w}x{h}"

# 頂點 NDC → pixel（跟 rasterizer 同一套換算），往質心內縮 12% 避開邊緣
verts = [(-0.8, -0.8, (255, 0, 0)), (0.8, -0.8, (0, 255, 0)), (0.0, 0.8, (0, 0, 255))]
cx = sum(v[0] for v in verts) / 3, sum(v[1] for v in verts) / 3
checks = []
for x, y, color in verts:
    sx, sy = x + (cx[0]-x)*0.12, y + (cx[1]-y)*0.12
    px_ = int((sx*0.5+0.5)*w); py_ = int((1-(sy*0.5+0.5))*h)
    checks.append(((px_, py_), color))
# 質心 ≈ 三色平均
gx = int((cx[0]*0.5+0.5)*w); gy = int((1-(cx[1]*0.5+0.5))*h)
checks.append(((gx, gy), (85, 85, 85)))
# 三角形外（角落）必須是黑透明
assert get(5, 5) == (0, 0, 0, 0), f"corner should be empty, got {get(5,5)}"

for (x, y), want in checks:
    got = get(x, y)
    assert near(got, want), f"pixel({x},{y}) = {got}, want ≈{want}"
print("check_pixels: all pass ✓")
