add_llvm_component_library(LLVMEMBERDesc
  EMBERAsmBackend.cpp
  EMBERBaseInfo.cpp
  EMBERELFObjectWriter.cpp
  EMBERInstPrinter.cpp
  EMBERMCAsmInfo.cpp
  EMBERMCCodeEmitter.cpp
  EMBERMCExpr.cpp
  EMBERMCTargetDesc.cpp
  EMBERMatInt.cpp
  EMBERTargetStreamer.cpp
  EMBERELFStreamer.cpp

  LINK_COMPONENTS
  MC
  EMBERInfo
  Support

  ADD_TO_COMPONENT
  EMBER
)
