; gradient triangle: pass interpolated RGB through, alpha=1
LDIN  R0, I0      ; r
LDIN  R1, I1      ; g
LDIN  R2, I2      ; b
LDI   R3, 1.0     ; alpha
STOUT O0, R0
STOUT O1, R1
STOUT O2, R2
STOUT O3, R3
RET
