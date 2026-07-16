add_llvm_component_library(LLVMToyGPUDesc
  ToyGPUMCTargetDesc.cpp
  ToyGPUInstPrinter.cpp
  LINK_COMPONENTS MC Support ToyGPUInfo TargetParser
  ADD_TO_COMPONENT ToyGPU
)
