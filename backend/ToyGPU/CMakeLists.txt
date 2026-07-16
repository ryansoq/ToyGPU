add_llvm_component_group(ToyGPU)

set(LLVM_TARGET_DEFINITIONS ToyGPU.td)

tablegen(LLVM ToyGPUGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM ToyGPUGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM ToyGPUGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM ToyGPUGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM ToyGPUGenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(ToyGPUCommonTableGen)

add_llvm_target(ToyGPUCodeGen
  ToyGPUAsmPrinter.cpp
  ToyGPUFrameLowering.cpp
  ToyGPUInstrInfo.cpp
  ToyGPUISelDAGToDAG.cpp
  ToyGPUISelLowering.cpp
  ToyGPURegisterInfo.cpp
  ToyGPUSubtarget.cpp
  ToyGPUTargetMachine.cpp

  LINK_COMPONENTS
  AsmPrinter CodeGen Core MC SelectionDAG Support Target TargetParser
  ToyGPUDesc ToyGPUInfo

  ADD_TO_COMPONENT ToyGPU
)

add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)
