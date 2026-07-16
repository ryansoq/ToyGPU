//===----------------------------------------------------------------------===//
// spirv2llvm — 把 fragment shader 的 SPIR-V 翻成 ToyGPU ABI 的 LLVM IR。
//
// 用法：spirv2llvm frag.spv frag.ll
//
// 為什麼自己寫 walker（不用 SPIRV-LLVM-Translator）：官方 translator 對
// graphics shader 支援不好，而且我們只需要處理漸層 shader 那十幾個 opcode。
// 自己走一遍反而看得清楚「SPIR-V → IR」到底發生什麼。
//
// 設計（two-pass）：
//   pass 1：掃過所有指令，記下 types / constants / decorations(Location) /
//           variables(StorageClass) —— 建 id → 資訊 的表。
//   pass 2：走 function body，把每個 SSA 值 scalarize 成 float，
//           對照 ToyGPU ABI 產生 LLVM IR：
//             Input  變數的 load/extract  → @toygpu.input(channel)
//             Output 變數的 store          → @toygpu.output(channel, val)
//
// 輸出用文字直接印（不 link LLVM）—— 保持工具零依賴、輸出一眼可讀。
// SPEC 原寫「用 IRBuilder」，這裡選文字輸出換取 clone-and-build 的簡單，
// 產物 .ll 與 IRBuilder 版等價。
//===----------------------------------------------------------------------===//
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

// 只需要的 SPIR-V opcode（值取自官方 spec；不引 spirv-headers 保持自足）
enum Op : uint16_t {
  OpName = 5, OpDecorate = 71, OpTypeVoid = 19, OpTypeFloat = 22,
  OpTypeVector = 23, OpTypePointer = 32, OpTypeFunction = 33,
  OpVariable = 59, OpConstant = 43, OpLoad = 61, OpStore = 62,
  OpCompositeExtract = 81, OpCompositeConstruct = 80,
  OpFunction = 54, OpLabel = 248, OpReturn = 253, OpFunctionEnd = 56,
  OpEntryPoint = 15, OpExtInstImport = 11, OpMemoryModel = 14,
  OpSource = 3, OpExecutionMode = 16, OpCapability = 17,
  OpTypeInt = 21,
};
enum StorageClass { SC_Input = 1, SC_Output = 3 };
enum Decoration { Dec_Location = 30 };

struct Word { uint16_t op; uint16_t wc; const uint32_t *ops; };

int main(int argc, char **argv) {
  if (argc != 3) { fprintf(stderr, "usage: spirv2llvm in.spv out.ll\n"); return 2; }

  // ── 讀 binary ──
  FILE *f = fopen(argv[1], "rb");
  if (!f) { fprintf(stderr, "cannot open %s\n", argv[1]); return 1; }
  std::vector<uint32_t> w;
  uint32_t x;
  while (fread(&x, 4, 1, f) == 1) w.push_back(x);
  fclose(f);
  if (w.size() < 5 || w[0] != 0x07230203) {
    fprintf(stderr, "not a SPIR-V binary (bad magic)\n"); return 1;
  }

  // ── 切成指令（從 header 之後，word[5]）──
  std::vector<Word> insts;
  for (size_t i = 5; i < w.size();) {
    uint16_t wc = w[i] >> 16, op = w[i] & 0xFFFF;
    if (wc == 0) break;
    insts.push_back({op, wc, &w[i + 1]});
    i += wc;
  }

  // ── pass 1：建 id 資訊表 ──
  std::map<uint32_t, int> floatWidth;             // id → OpTypeFloat 寬度
  std::map<uint32_t, uint32_t> vecComp;           // vector type id → 元素數
  std::map<uint32_t, uint32_t> ptrStorage;        // pointer type id → StorageClass
  std::map<uint32_t, uint32_t> ptrPointee;        // pointer type id → 被指型別 id
  std::map<uint32_t, uint32_t> varPtrType;        // variable id → pointer type id
  std::map<uint32_t, uint32_t> varStorage;        // variable id → StorageClass
  std::map<uint32_t, uint32_t> varLocation;       // variable id → Location decoration
  std::map<uint32_t, float> constF;               // id → float 常數值

  for (auto &in : insts) {
    switch (in.op) {
    case OpTypeFloat: floatWidth[in.ops[0]] = in.ops[1]; break;
    case OpTypeVector: vecComp[in.ops[0]] = in.ops[2]; break;
    case OpTypePointer:
      ptrStorage[in.ops[0]] = in.ops[1];
      ptrPointee[in.ops[0]] = in.ops[2];
      break;
    case OpVariable: {                            // ops: resultType, resultId, SC
      uint32_t ptrType = in.ops[0], id = in.ops[1], sc = in.ops[2];
      varPtrType[id] = ptrType; varStorage[id] = sc;
      break;
    }
    case OpConstant: {                            // ops: type, id, value
      float v; uint32_t bits = in.ops[2]; memcpy(&v, &bits, 4);
      constF[in.ops[1]] = v; break;
    }
    case OpDecorate:
      if (in.wc >= 4 && in.ops[1] == Dec_Location)
        varLocation[in.ops[0]] = in.ops[2];
      break;
    default: break;
    }
  }

  // ── pass 2：走 function body，emit LLVM IR ──
  // scalarVal：SPIR-V id → 我們在 IR 裡承接它的表示式字串（scalar float）
  std::map<uint32_t, std::string> scalarVal;
  // vecChannels：某個 vector id 拆成的 scalar 表示式清單
  std::map<uint32_t, std::vector<std::string>> vecChannels;

  std::string body;
  int tmp = 0;
  auto fresh = [&] { return "%t" + std::to_string(tmp++); };

  for (auto &in : insts) {
    switch (in.op) {
    case OpLoad: {                                // resultType, id, pointer
      uint32_t id = in.ops[1], ptr = in.ops[2];
      if (varStorage.count(ptr) && varStorage[ptr] == SC_Input) {
        // 讀 Input 變數：scalarize 成 N 個 @toygpu.input(loc*?+ch)
        uint32_t comps = vecComp.count(ptrPointee[varPtrType[ptr]])
                             ? vecComp[ptrPointee[varPtrType[ptr]]] : 1;
        std::vector<std::string> chans;
        for (uint32_t c = 0; c < comps; c++) {
          std::string t = fresh();
          body += "  " + t + " = call float @toygpu.input(i32 " +
                  std::to_string(c) + ")\n";
          chans.push_back(t);
        }
        vecChannels[id] = chans;
      }
      break;
    }
    case OpCompositeExtract: {                    // type, id, composite, index
      uint32_t id = in.ops[1], comp = in.ops[2], idx = in.ops[3];
      if (vecChannels.count(comp)) scalarVal[id] = vecChannels[comp][idx];
      break;
    }
    case OpCompositeConstruct: {                  // type, id, comp0..N
      std::vector<std::string> chans;
      for (uint16_t k = 2; k < in.wc - 1; k++) {
        uint32_t c = in.ops[k];
        if (scalarVal.count(c)) chans.push_back(scalarVal[c]);
        else if (constF.count(c)) {
          char buf[32]; snprintf(buf, sizeof buf, "%.8e", constF[c]);
          chans.push_back(std::string("float ") + buf);
        } else chans.push_back("float 0.0");
      }
      vecChannels[in.ops[1]] = chans;
      break;
    }
    case OpStore: {                               // pointer, object
      uint32_t ptr = in.ops[0], obj = in.ops[1];
      if (varStorage.count(ptr) && varStorage[ptr] == SC_Output &&
          vecChannels.count(obj)) {
        auto &chans = vecChannels[obj];
        for (size_t c = 0; c < chans.size(); c++) {
          std::string v = chans[c];
          // chans[c] 可能是 "%tN"（要當 float SSA）或 "float X.X"（常數）
          std::string arg = (v.rfind("float ", 0) == 0) ? v : ("float " + v);
          body += "  call void @toygpu.output(i32 " + std::to_string(c) +
                  ", " + arg + ")\n";
        }
      }
      break;
    }
    default: break;
    }
  }

  // ── 輸出 .ll ──
  FILE *o = fopen(argv[2], "w");
  if (!o) { fprintf(stderr, "cannot write %s\n", argv[2]); return 1; }
  fprintf(o,
    "; generated by spirv2llvm from %s\n"
    "declare float @toygpu.input(i32)\n"
    "declare float @toygpu.uniform(i32)\n"
    "declare void @toygpu.output(i32, float)\n\n"
    "define void @main() {\n"
    "entry:\n"
    "%s"
    "  ret void\n"
    "}\n",
    argv[1], body.c_str());
  fclose(o);
  printf("spirv2llvm: %zu instructions -> %s\n", insts.size(), argv[2]);
  return 0;
}
