add_llvm_component_library(LLVMEMBERDisassembler
  EMBERDisassembler.cpp

  LINK_COMPONENTS
  MC
  MCDisassembler
  EMBERDesc
  EMBERInfo
  Support

  ADD_TO_COMPONENT
  EMBER
  )
