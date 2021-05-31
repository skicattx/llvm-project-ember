//===-- EMBERMCTargetDesc.cpp - EMBER Target Descriptions -----------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
///
/// This file provides EMBER-specific target descriptions.
///
//===----------------------------------------------------------------------===//

#include "EMBERMCTargetDesc.h"
#include "EMBERBaseInfo.h"
#include "EMBERELFStreamer.h"
#include "EMBERInstPrinter.h"
#include "EMBERMCAsmInfo.h"
#include "EMBERTargetStreamer.h"
#include "TargetInfo/EMBERTargetInfo.h"
// #include "llvm/ADT/STLExtras.h"
// #include "llvm/MC/MCAsmInfo.h"
// #include "llvm/MC/MCInstrAnalysis.h"
#include "llvm/MC/MCInstrInfo.h"
// #include "llvm/MC/MCRegisterInfo.h"
// #include "llvm/MC/MCStreamer.h"
// #include "llvm/MC/MCSubtargetInfo.h"
// #include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "EMBERGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "EMBERGenRegisterInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "EMBERGenSubtargetInfo.inc"

using namespace llvm;

static MCInstrInfo *createEMBERMCInstrInfo() 
{
    MCInstrInfo* X = new MCInstrInfo();
    InitEMBERMCInstrInfo(X);
    return X;
}

static MCRegisterInfo *createEMBERMCRegisterInfo(const Triple &TT)
{
  MCRegisterInfo *X = new MCRegisterInfo();
  InitEMBERMCRegisterInfo(X, EMBER::R0);
  return X;
}

static MCAsmInfo *createEMBERMCAsmInfo(const MCRegisterInfo &MRI,
                                       const Triple &TT,
                                       const MCTargetOptions &Options) 
{
  MCAsmInfo *MAI = new EMBERMCAsmInfo(TT);

  MCRegister SP = MRI.getDwarfRegNum(EMBER::SP, true);
  MCCFIInstruction Inst = MCCFIInstruction::cfiDefCfa(nullptr, SP, 0);
  MAI->addInitialFrameState(Inst);

  return MAI;
}

static MCSubtargetInfo *createEMBERMCSubtargetInfo(const Triple &TT, StringRef CPU, StringRef FS)
{
    if (TT.isArch64Bit())
        report_fatal_error(Twine("64-bit CPU is not supported. Use 'ember32'"));
    if (CPU == "generic")
        report_fatal_error(Twine("CPU 'generic' is not supported. Use 'ember32'"));
    if (CPU.empty())
        CPU = "ember32";
    return createEMBERMCSubtargetInfoImpl(TT, CPU, /*TuneCPU*/ CPU, FS);
}

static MCInstPrinter *createEMBERMCInstPrinter(const Triple &T,
                                               unsigned SyntaxVariant,
                                               const MCAsmInfo &MAI,
                                               const MCInstrInfo &MII,
                                               const MCRegisterInfo &MRI) 
{
    return new EMBERInstPrinter(MAI, MII, MRI);
}

static MCTargetStreamer *createEMBERObjectTargetStreamer(MCStreamer &S, const MCSubtargetInfo &STI) 
{
    const Triple &TT = STI.getTargetTriple();
    if (TT.isOSBinFormatELF())
        return new EMBERTargetELFStreamer(S, STI);
    return nullptr;
}

static MCTargetStreamer *createEMBERAsmTargetStreamer(MCStreamer            &S,
                                                      formatted_raw_ostream &OS,
                                                      MCInstPrinter         *InstPrint,
                                                      bool                  isVerboseAsm) 
{
    return new EMBERTargetAsmStreamer(S, OS);
}

/*
static MCTargetStreamer *createEMBERNullTargetStreamer(MCStreamer &S) {
  return new EMBERTargetStreamer(S);
}

namespace {

class EMBERMCInstrAnalysis : public MCInstrAnalysis {
public:
  explicit EMBERMCInstrAnalysis(const MCInstrInfo *Info)
      : MCInstrAnalysis(Info) {}

  bool evaluateBranch(const MCInst &Inst, uint64_t Addr, uint64_t Size,
                      uint64_t &Target) const override {
    if (isConditionalBranch(Inst)) {
      int64_t Imm;
      if (Size == 2)
        Imm = Inst.getOperand(1).getImm();
      else
        Imm = Inst.getOperand(2).getImm();
      Target = Addr + Imm;
      return true;
    }

    if (Inst.getOpcode() == EMBER::C_JAL || Inst.getOpcode() == EMBER::C_J) {
      Target = Addr + Inst.getOperand(0).getImm();
      return true;
    }

    if (Inst.getOpcode() == EMBER::JAL) {
      Target = Addr + Inst.getOperand(1).getImm();
      return true;
    }

    return false;
  }
};

} // end anonymous namespace

static MCInstrAnalysis *createEMBERInstrAnalysis(const MCInstrInfo *Info) {
  return new EMBERMCInstrAnalysis(Info);
}
*/
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEMBERTargetMC() 
{
    TargetRegistry::RegisterMCAsmInfo(getTheEMBER32Target(), createEMBERMCAsmInfo);
    TargetRegistry::RegisterMCInstrInfo(getTheEMBER32Target(), createEMBERMCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(getTheEMBER32Target(), createEMBERMCRegisterInfo);
    TargetRegistry::RegisterMCAsmBackend(getTheEMBER32Target(), createEMBERAsmBackend);
    TargetRegistry::RegisterMCCodeEmitter(getTheEMBER32Target(), createEMBERMCCodeEmitter);
    TargetRegistry::RegisterMCInstPrinter(getTheEMBER32Target(), createEMBERMCInstPrinter);
    TargetRegistry::RegisterMCSubtargetInfo(getTheEMBER32Target(), createEMBERMCSubtargetInfo);
    TargetRegistry::RegisterObjectTargetStreamer(getTheEMBER32Target(), createEMBERObjectTargetStreamer);
//     TargetRegistry::RegisterMCInstrAnalysis(getTheEMBER32Target(), createEMBERInstrAnalysis);

    // Register the asm target streamer.
    TargetRegistry::RegisterAsmTargetStreamer(getTheEMBER32Target(), createEMBERAsmTargetStreamer);
    // Register the null target streamer.
//     TargetRegistry::RegisterNullTargetStreamer(getTheEMBER32Target(), createEMBERNullTargetStreamer);
}
