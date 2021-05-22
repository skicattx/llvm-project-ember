add_llvm_component_library(LLVMEMBERAsmParser
  EMBERAsmParser.cpp

  LINK_COMPONENTS
  MC
  MCParser
  EMBERDesc
  EMBERInfo
  Support

  ADD_TO_COMPONENT
  EMBER
  )
