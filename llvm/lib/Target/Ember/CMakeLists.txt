add_llvm_component_group(EMBER)

set(LLVM_TARGET_DEFINITIONS EMBER.td)

tablegen(LLVM EMBERGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM EMBERGenAsmWriter.inc -gen-asm-writer)
#tablegen(LLVM EMBERGenCallingConv.inc -gen-callingconv)
#tablegen(LLVM EMBERGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM EMBERGenDisassemblerTables.inc -gen-disassembler)
#tablegen(LLVM EMBERGenFastISel.inc -gen-fast-isel)
#tablegen(LLVM EMBERGenGlobalISel.inc -gen-global-isel)
tablegen(LLVM EMBERGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM EMBERGenMCCodeEmitter.inc -gen-emitter)
#tablegen(LLVM EMBERGenMCPseudoLowering.inc -gen-pseudo-lowering)
#tablegen(LLVM EMBERGenRegisterBank.inc -gen-register-bank)
tablegen(LLVM EMBERGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM EMBERGenSubtargetInfo.inc -gen-subtarget)
#tablegen(LLVM EMBERGenExegesis.inc -gen-exegesis)

add_public_tablegen_target(EMBERCommonTableGen)

add_llvm_target(EmberCodeGen
  EMBERAsmPrinter.cpp
  EMBERInstrInfo.cpp
#  EMBERISelLowering.cpp
  EMBERFrameLowering.cpp
#  EMBERMachineFunctionInfo.cpp
  EMBERRegisterInfo.cpp
  EMBERSubtarget.cpp
  EMBERTargetMachine.cpp
#  EMBERMCInstLower.cpp
  EMBERTargetObjectFile.cpp

  LINK_COMPONENTS
  Analysis
  AsmPrinter
  Core
  CodeGen
  MC
  EMBERDesc
  EMBERInfo
  Support
  Target

  ADD_TO_COMPONENT
  EMBER
  )
#SelectionDAG

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
