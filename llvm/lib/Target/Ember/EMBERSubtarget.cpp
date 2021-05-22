//===-- EMBERSubtarget.cpp - EMBER Subtarget Information --------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file implements the EMBER specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "EMBERSubtarget.h"
#include "EMBER.h"
//#include "EMBERMachineFunction.h"
#include "EMBERRegisterInfo.h"
#include "EMBERTargetMachine.h"
// #include "EMBERCallLowering.h"
// #include "EMBERLegalizerInfo.h"
// #include "EMBERRegisterBankInfo.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "EMBER-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "EMBERGenSubtargetInfo.inc"

// FIXME: Maybe this should be on by default when EMBER16 is specified
//
static cl::opt<bool>
    Mixed16_32("EMBER-mixed-16-32", cl::init(false),
               cl::desc("Allow for a mixture of EMBER16 "
                        "and EMBER32 code in a single output file"),
               cl::Hidden);

static cl::opt<bool> EMBER_Os16("EMBER-os16", cl::init(false),
                               cl::desc("Compile all functions that don't use "
                                        "floating point as EMBER 16"),
                               cl::Hidden);

static cl::opt<bool> EMBER16HardFloat("EMBER16-hard-float", cl::NotHidden,
                                     cl::desc("Enable EMBER16 hard float."),
                                     cl::init(false));

static cl::opt<bool>
    EMBER16ConstantIslands("EMBER16-constant-islands", cl::NotHidden,
                          cl::desc("Enable EMBER16 constant islands."),
                          cl::init(true));

static cl::opt<bool>
    GPOpt("mgpopt", cl::Hidden,
          cl::desc("Enable gp-relative addressing of EMBER small data items"));

// bool EMBERSubtarget::DspWarningPrinted = false;
// bool EMBERSubtarget::MSAWarningPrinted = false;
// bool EMBERSubtarget::VirtWarningPrinted = false;
// bool EMBERSubtarget::CRCWarningPrinted = false;
// bool EMBERSubtarget::GINVWarningPrinted = false;

void EMBERSubtarget::anchor() {}

EMBERSubtarget::EMBERSubtarget(const Triple &TT, StringRef CPU, StringRef FS, const EMBERTargetMachine &TM)
    : EMBERGenSubtargetInfo(TT, CPU, CPU, FS), TM(TM),
      //       EMBERArchVersion(EMBERDefault), IsLittle(little), IsSoftFloat(false),
//       IsSingleFloat(false), IsFPXX(false), NoABICalls(false), Abs2008(false),
//       IsFP64bit(false), UseOddSPReg(true), IsNaN2008bit(false),
//       IsGP64bit(false), HasVFPU(false), HasCnEMBER(false), HasCnEMBERP(false),
//       HasEMBER3_32(false), HasEMBER3_32r2(false), HasEMBER4_32(false),
//       HasEMBER4_32r2(false), HasEMBER5_32r2(false), InEMBER16Mode(false),
//       InEMBER16HardFloat(EMBER16HardFloat), InMicroEMBERMode(false), HasDSP(false),
//       HasDSPR2(false), HasDSPR3(false), AllowMixed16_32(Mixed16_32 | EMBER_Os16),
//       Os16(EMBER_Os16), HasMSA(false), UseTCCInDIV(false), HasSym32(false),
//       HasEVA(false), DisableMadd4(false), HasMT(false), HasCRC(false),
//       HasVirt(false), HasGINV(false), UseIndirectJumpsHazard(false),
//       StackAlignOverride(StackAlignOverride), TM(TM), TargetTriple(TT),
      TSInfo(), InstrInfo(EMBERInstrInfo::create(
                    initializeSubtargetDependencies(CPU, FS, TM)))/*,
      FrameLowering(EMBERFrameLowering::create(*this)),
      TLInfo(EMBERTargetLowering::create(TM, *this)) */{

//   if (EMBERArchVersion == EMBERDefault)
//     EMBERArchVersion = EMBER32;

  // Don't even attempt to generate code for EMBER-I and EMBER-V. They have not
/*
  // been tested and currently exist for the integrated assembler only.
  if (EMBERArchVersion == EMBER1)
    report_fatal_error("Code generation for EMBER-I is not implemented", false);
  if (EMBERArchVersion == EMBER5)
    report_fatal_error("Code generation for EMBER-V is not implemented", false);

  // Check if Architecture and ABI are compatible.
  assert(((!isGP64bit() && isABI_O32()) ||
          (isGP64bit() && (isABI_N32() || isABI_N64()))) &&
         "Invalid  Arch & ABI pair.");

  if (hasMSA() && !isFP64bit())
    report_fatal_error("MSA requires a 64-bit FPU register file (FR=1 mode). "
                       "See -mattr=+fp64.",
                       false);

  if (isFP64bit() && !hasEMBER64() && hasEMBER32() && !hasEMBER32r2())
    report_fatal_error(
        "FPU with 64-bit registers is not available on EMBER32 pre revision 2. "
        "Use -mcpu=EMBER32r2 or greater.");

  if (!isABI_O32() && !useOddSPReg())
    report_fatal_error("-mattr=+nooddspreg requires the O32 ABI.", false);

  if (IsFPXX && (isABI_N32() || isABI_N64()))
    report_fatal_error("FPXX is not permitted for the N32/N64 ABI's.", false);

  if (hasEMBER64r6() && InMicroEMBERMode)
    report_fatal_error("microEMBER64R6 is not supported", false);

  if (!isABI_O32() && InMicroEMBERMode)
    report_fatal_error("microEMBER64 is not supported.", false);

  if (UseIndirectJumpsHazard) {
    if (InMicroEMBERMode)
      report_fatal_error(
          "cannot combine indirect jumps with hazard barriers and microEMBER");
    if (!hasEMBER32r2())
      report_fatal_error(
          "indirect jumps with hazard barriers requires EMBER32R2 or later");
  }
  if (inAbs2008Mode() && hasEMBER32() && !hasEMBER32r2()) {
    report_fatal_error("IEEE 754-2008 abs.fmt is not supported for the given "
                       "architecture.",
                       false);
  }

  if (hasEMBER32r6()) {
    StringRef ISA = hasEMBER64r6() ? "EMBER64r6" : "EMBER32r6";

    assert(isFP64bit());
    assert(isNaN2008());
    assert(inAbs2008Mode());
    if (hasDSP())
      report_fatal_error(ISA + " is not compatible with the DSP ASE", false);
  }

  if (NoABICalls && TM.isPositionIndependent())
    report_fatal_error("position-independent code requires '-mabicalls'");

  if (isABI_N64() && !TM.isPositionIndependent() && !hasSym32())
    NoABICalls = true;

  // Set UseSmallSection.
  UseSmallSection = GPOpt;
  if (!NoABICalls && GPOpt) {
    errs() << "warning: cannot use small-data accesses for '-mabicalls'"
           << "\n";
    UseSmallSection = false;
  }

  if (hasDSPR2() && !DspWarningPrinted) {
    if (hasEMBER64() && !hasEMBER64r2()) {
      errs() << "warning: the 'dspr2' ASE requires EMBER64 revision 2 or "
             << "greater\n";
      DspWarningPrinted = true;
    } else if (hasEMBER32() && !hasEMBER32r2()) {
      errs() << "warning: the 'dspr2' ASE requires EMBER32 revision 2 or "
             << "greater\n";
      DspWarningPrinted = true;
    }
  } else if (hasDSP() && !DspWarningPrinted) {
    if (hasEMBER64() && !hasEMBER64r2()) {
      errs() << "warning: the 'dsp' ASE requires EMBER64 revision 2 or "
             << "greater\n";
      DspWarningPrinted = true;
    } else if (hasEMBER32() && !hasEMBER32r2()) {
      errs() << "warning: the 'dsp' ASE requires EMBER32 revision 2 or "
             << "greater\n";
      DspWarningPrinted = true;
    }
  }

  StringRef ArchName = hasEMBER64() ? "EMBER64" : "EMBER32";

  if (!hasEMBER32r5() && hasMSA() && !MSAWarningPrinted) {
    errs() << "warning: the 'msa' ASE requires " << ArchName
           << " revision 5 or greater\n";
    MSAWarningPrinted = true;
  }
  if (!hasEMBER32r5() && hasVirt() && !VirtWarningPrinted) {
    errs() << "warning: the 'virt' ASE requires " << ArchName
           << " revision 5 or greater\n";
    VirtWarningPrinted = true;
  }
  if (!hasEMBER32r6() && hasCRC() && !CRCWarningPrinted) {
    errs() << "warning: the 'crc' ASE requires " << ArchName
           << " revision 6 or greater\n";
    CRCWarningPrinted = true;
  }
  if (!hasEMBER32r6() && hasGINV() && !GINVWarningPrinted) {
    errs() << "warning: the 'ginv' ASE requires " << ArchName
           << " revision 6 or greater\n";
    GINVWarningPrinted = true;
  }

  CallLoweringInfo.reset(new EMBERCallLowering(*getTargetLowering()));
  Legalizer.reset(new EMBERLegalizerInfo(*this));

  auto *RBI = new EMBERRegisterBankInfo(*getRegisterInfo());
  RegBankInfo.reset(RBI);
  InstSelector.reset(createEMBERInstructionSelector(
      *static_cast<const EMBERTargetMachine *>(&TM), *this, *RBI));
*/
}

bool EMBERSubtarget::isPositionIndependent() const {
  return TM.isPositionIndependent();
}

/// This overrides the PostRAScheduler bit in the SchedModel for any CPU.
/*
bool EMBERSubtarget::enablePostRAScheduler() const { return true; }

void EMBERSubtarget::getCriticalPathRCs(RegClassVector &CriticalPathRCs) const {
  CriticalPathRCs.clear();
  CriticalPathRCs.push_back(isGP64bit() ? &EMBER::GPR64RegClass
                                        : &EMBER::GPR32RegClass);
}
*/

CodeGenOpt::Level EMBERSubtarget::getOptLevelToEnablePostRAScheduler() const {
  return CodeGenOpt::Aggressive;
}

EMBERSubtarget &
EMBERSubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                               const TargetMachine &TM) {
/*
  StringRef CPUName = EMBER_MC::selectEMBERCPU(TM.getTargetTriple(), CPU);

  // Parse features string.
  ParseSubtargetFeatures(CPUName, / *TuneCPU* / CPUName, FS);
  // Initialize scheduling itinerary for the specified CPU.
  InstrItins = getInstrItineraryForCPU(CPUName);

  if (InEMBER16Mode && !IsSoftFloat)
    InEMBER16HardFloat = true;

  if (StackAlignOverride)
    stackAlignment = *StackAlignOverride;
  else if (isABI_N32() || isABI_N64())
    stackAlignment = Align(16);
  else {
    assert(isABI_O32() && "Unknown ABI for stack alignment!");
    stackAlignment = Align(8);
  }

  if ((isABI_N32() || isABI_N64()) && !isGP64bit())
    report_fatal_error("64-bit code requested on a subtarget that doesn't "
                       "support it!");*/

  return *this;
}

bool EMBERSubtarget::useConstantIslands() {
  LLVM_DEBUG(dbgs() << "use constant islands " << EMBER16ConstantIslands
                    << "\n");
  return EMBER16ConstantIslands;
}

Reloc::Model EMBERSubtarget::getRelocationModel() const {
  return TM.getRelocationModel();
}

/*
bool EMBERSubtarget::isABI_N64() const { return getABI().IsN64(); }
bool EMBERSubtarget::isABI_N32() const { return getABI().IsN32(); }
bool EMBERSubtarget::isABI_O32() const { return getABI().IsO32(); }
const EMBERABIInfo &EMBERSubtarget::getABI() const { return TM.getABI(); }
*/

const CallLowering *EMBERSubtarget::getCallLowering() const {
  return CallLoweringInfo.get();
}

const LegalizerInfo *EMBERSubtarget::getLegalizerInfo() const {
  return Legalizer.get();
}

const RegisterBankInfo *EMBERSubtarget::getRegBankInfo() const {
  return RegBankInfo.get();
}

InstructionSelector *EMBERSubtarget::getInstructionSelector() const {
  return InstSelector.get();
}
