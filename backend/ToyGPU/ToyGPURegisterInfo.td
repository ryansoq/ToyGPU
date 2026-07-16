//===-- ToyGPURegisterInfo.td - 暫存器定義 ------------------*- tablegen -*-===//
class ToyGPUReg<bits<16> enc, string n> : Register<n> {
  let HWEncoding = enc;
  let Namespace = "ToyGPU";
}

foreach i = 0-15 in
  def R#i : ToyGPUReg<i, "R"#i>;

// 唯一的 register class：16 × f32。
// I/U/O 刻意不建模成暫存器（見 isa.md 設計筆記）。
// 注意 [f32, i32]：不是因為我們要整數運算 —— TableGen 共用的
// TargetSelectionDAG.td 有 atomic PatFrags 要求 target 至少存在一種
// 整數型別，純 f32 的 register file 會在 tblgen 直接報錯
// （"Type set is empty for each HW mode"）。這是極簡 target 的
// 經典入坑點：掛個 i32 上去讓型別推導有東西可用即可。
def FPR : RegisterClass<"ToyGPU", [f32, i32], 32, (sequence "R%u", 0, 15)>;
