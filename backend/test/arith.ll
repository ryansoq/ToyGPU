; RUN: llc -mtriple=toygpu < %s | FileCheck %s
declare float @toygpu.input(i32)
declare void @toygpu.output(i32, float)
define void @main() {
  %a = call float @toygpu.input(i32 0)
  %b = call float @toygpu.input(i32 1)
  %s = fadd float %a, %b
  %d = fsub float %s, %a
  call void @toygpu.output(i32 0, float %d)
  ret void
}
; CHECK: LDIN R0, I0
; CHECK: LDIN R1, I1
; CHECK: FADD
; CHECK: FSUB
; CHECK: STOUT O0
; CHECK: RET
