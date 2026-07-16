; RUN: llc -mtriple=toygpu < %s | FileCheck %s
declare float @toygpu.input(i32)
declare void @toygpu.output(i32, float)
define void @main() {
entry:
  %r = call float @toygpu.input(i32 0)
  %g = call float @toygpu.input(i32 1)
  %b = call float @toygpu.input(i32 2)
  call void @toygpu.output(i32 0, float %r)
  call void @toygpu.output(i32 1, float %g)
  call void @toygpu.output(i32 2, float %b)
  call void @toygpu.output(i32 3, float 1.0)
  ret void
}
; CHECK:      LDIN R0, I0
; CHECK-NEXT: LDIN R1, I1
; CHECK-NEXT: LDIN R2, I2
; CHECK:      STOUT O0, R0
; CHECK:      LDI R0, 1
; CHECK:      STOUT O3, R0
; CHECK:      RET
