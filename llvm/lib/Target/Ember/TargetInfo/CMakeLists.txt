add_llvm_component_library(LLVMEMBERInfo
  EMBERTargetInfo.cpp

  LINK_COMPONENTS
  Support

  ADD_TO_COMPONENT
  EMBER
  )
