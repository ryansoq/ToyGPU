add_llvm_component_library(LLVMToyGPUInfo
  ToyGPUTargetInfo.cpp
  LINK_COMPONENTS MC Support TargetParser
  ADD_TO_COMPONENT ToyGPU
)
