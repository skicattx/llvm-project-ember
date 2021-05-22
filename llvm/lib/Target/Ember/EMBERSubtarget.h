//===-- EMBERSubtarget.h - Define Subtarget for the EMBER ---------*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file declares the EMBER specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_EMBERSUBTARGET_H
#define LLVM_LIB_TARGET_EMBER_EMBERSUBTARGET_H

//#include "MCTargetDesc/EMBERABIInfo.h"
#include "EMBERFrameLowering.h"
// #include "EMBERISelLowering.h"
#include "EMBERInstrInfo.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/GlobalISel/CallLowering.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/GlobalISel/RegisterBankInfo.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCInstrItineraries.h"
#include "llvm/Support/ErrorHandling.h"
#include <string>

#define GET_SUBTARGETINFO_HEADER
#include "EMBERGenSubtargetInfo.inc"

namespace llvm {
class StringRef;

class EMBERTargetMachine;

class EMBERSubtarget : public EMBERGenSubtargetInfo {
  virtual void anchor();

  enum EMBERArchEnum {
    EMBERDefault,
    EMBER32
  };

  enum class CPU { Ember};
  /*
  // Used to avoid printing dsp warnings multiple times.
  static bool DspWarningPrinted;

  // Used to avoid printing msa warnings multiple times.
  static bool MSAWarningPrinted;

  // Used to avoid printing crc warnings multiple times.
  static bool CRCWarningPrinted;

  // Used to avoid printing ginv warnings multiple times.
  static bool GINVWarningPrinted;

  // Used to avoid printing virt warnings multiple times.
  static bool VirtWarningPrinted;

  // EMBER architecture version
  EMBERArchEnum EMBERArchVersion;

  // Processor implementation (unused but required to exist by
  // tablegen-erated code).
  CPU ProcImpl;

  // IsLittle - The target is Little Endian
  bool IsLittle;

  // IsSoftFloat - The target does not support any floating point instructions.
  bool IsSoftFloat;

  // IsSingleFloat - The target only supports single precision float
  // point operations. This enable the target to use all 32 32-bit
  // floating point registers instead of only using even ones.
  bool IsSingleFloat;

  // IsFPXX - EMBER O32 modeless ABI.
  bool IsFPXX;

  // NoABICalls - Disable SVR4-style position-independent code.
  bool NoABICalls;

  // Abs2008 - Use IEEE 754-2008 abs.fmt instruction.
  bool Abs2008;

  // IsFP64bit - The target processor has 64-bit floating point registers.
  bool IsFP64bit;

  /// Are odd single-precision registers permitted?
  /// This corresponds to -modd-spreg and -mno-odd-spreg
  bool UseOddSPReg;

  // IsNan2008 - IEEE 754-2008 NaN encoding.
  bool IsNaN2008bit;

  // IsGP64bit - General-purpose registers are 64 bits wide
  bool IsGP64bit;

  // IsPTR64bit - Pointers are 64 bit wide
  bool IsPTR64bit;

  // HasVFPU - Processor has a vector floating point unit.
  bool HasVFPU;

  // CPU supports cnEMBER (Cavium Networks Octeon CPU).
  bool HasCnEMBER;

  // CPU supports cnEMBERP (Cavium Networks Octeon+ CPU).
  bool HasCnEMBERP;

  // isLinux - Target system is Linux. Is false we consider ELFOS for now.
  bool IsLinux;

  // UseSmallSection - Small section is used.
  bool UseSmallSection;

  /// Features related to the presence of specific instructions.

  // HasEMBER3_32 - The subset of EMBER-III instructions added to EMBER32
  bool HasEMBER3_32;

  // HasEMBER3_32r2 - The subset of EMBER-III instructions added to EMBER32r2
  bool HasEMBER3_32r2;

  // HasEMBER4_32 - Has the subset of EMBER-IV present in EMBER32
  bool HasEMBER4_32;

  // HasEMBER4_32r2 - Has the subset of EMBER-IV present in EMBER32r2
  bool HasEMBER4_32r2;

  // HasEMBER5_32r2 - Has the subset of EMBER-V present in EMBER32r2
  bool HasEMBER5_32r2;

  // InEMBER16 -- can process EMBER16 instructions
  bool InEMBER16Mode;

  // EMBER16 hard float
  bool InEMBER16HardFloat;

  // InMicroEMBER -- can process MicroEMBER instructions
  bool InMicroEMBERMode;

  // HasDSP, HasDSPR2, HasDSPR3 -- supports DSP ASE.
  bool HasDSP, HasDSPR2, HasDSPR3;

  // Has3D -- Supports EMBER3D ASE.
  bool Has3D;

  // Allow mixed EMBER16 and EMBER32 in one source file
  bool AllowMixed16_32;

  // Optimize for space by compiling all functions as EMBER 16 unless
  // it needs floating point. Functions needing floating point are
  // compiled as EMBER32
  bool Os16;

  // HasMSA -- supports MSA ASE.
  bool HasMSA;

  // UseTCCInDIV -- Enables the use of trapping in the assembler.
  bool UseTCCInDIV;

  // Sym32 -- On EMBER64 symbols are 32 bits.
  bool HasSym32;

  // HasEVA -- supports EVA ASE.
  bool HasEVA;

  // nomadd4 - disables generation of 4-operand madd.s, madd.d and
  // related instructions.
  bool DisableMadd4;

  // HasMT -- support MT ASE.
  bool HasMT;

  // HasCRC -- supports R6 CRC ASE
  bool HasCRC;

  // HasVirt -- supports Virtualization ASE
  bool HasVirt;

  // HasGINV -- supports R6 Global INValidate ASE
  bool HasGINV;

  // Use hazard variants of the jump register instructions for indirect
  // function calls and jump tables.
  bool UseIndirectJumpsHazard;

  // Disable use of the `jal` instruction.
  bool UseLongCalls = false;

  // Assume 32-bit GOT.
  bool UseXGOT = false;

*/
  /// The minimum alignment known to hold of the stack frame on
  /// entry to the function and which must be maintained by every function.
  Align stackAlignment;

  /// The overridden stack alignment.
  MaybeAlign StackAlignOverride;

  InstrItineraryData InstrItins;

  // We can override the determination of whether we are in EMBER16 mode
  // as from the command line
  enum {NoOverride, EMBER16Override, NoEMBER16Override} OverrideMode;

  const EMBERTargetMachine &TM;

  Triple TargetTriple;

  const SelectionDAGTargetInfo TSInfo;
  std::unique_ptr<const EMBERInstrInfo> InstrInfo;
  std::unique_ptr<const EMBERFrameLowering> FrameLowering;
//  std::unique_ptr<const EMBERTargetLowering> TLInfo;

public:
  bool isPositionIndependent() const;
  /// This overrides the PostRAScheduler bit in the SchedModel for each CPU.
  bool enablePostRAScheduler() const override;
  void getCriticalPathRCs(RegClassVector &CriticalPathRCs) const override;
  CodeGenOpt::Level getOptLevelToEnablePostRAScheduler() const override;

//   bool isABI_N64() const;
//   bool isABI_N32() const;
//   bool isABI_O32() const;
// //  const EMBERABIInfo &getABI() const;
//   bool isABI_FPXX() const { return isABI_O32() && IsFPXX; }

  /// This constructor initializes the data members to match that
  /// of the specified triple.
  EMBERSubtarget(const Triple &TT, StringRef CPU, StringRef FS, const EMBERTargetMachine &TM);

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

  bool hasEMBER32() const {
    return true;
  }

  bool isLittle() const { return true; }
//   bool isABICalls() const { return !NoABICalls; }
//   bool isFPXX() const { return IsFPXX; }
//   bool isFP64bit() const { return IsFP64bit; }
//   bool useOddSPReg() const { return UseOddSPReg; }
//   bool noOddSPReg() const { return !UseOddSPReg; }
//   bool isNaN2008() const { return IsNaN2008bit; }
//   bool inAbs2008Mode() const { return Abs2008; }
//   bool isGP64bit() const { return IsGP64bit; }
//   bool isGP32bit() const { return !IsGP64bit; }
//   unsigned getGPRSizeInBytes() const { return isGP64bit() ? 8 : 4; }
//   bool isPTR64bit() const { return IsPTR64bit; }
//   bool isPTR32bit() const { return !IsPTR64bit; }
//   bool hasSym32() const {
//     return (HasSym32 && isABI_N64()) || isABI_N32() || isABI_O32();
//   }
//   bool isSingleFloat() const { return IsSingleFloat; }
//   bool isTargetELF() const { return TargetTriple.isOSBinFormatELF(); }
//   bool hasVFPU() const { return HasVFPU; }
//   bool inEMBER16Mode() const { return InEMBER16Mode; }
//   bool inEMBER16ModeDefault() const {
//     return InEMBER16Mode;
//   }
//   // Hard float for EMBER16 means essentially to compile as soft float
//   // but to use a runtime library for soft float that is written with
//   // native EMBER32 floating point instructions (those runtime routines
//   // run in EMBER32 hard float mode).
//   bool inEMBER16HardFloat() const {
//     return inEMBER16Mode() && InEMBER16HardFloat;
//   }
//   bool inMicroEMBERMode() const { return InMicroEMBERMode && !InEMBER16Mode; }
//   bool inMicroEMBER32r6Mode() const {
//     return inMicroEMBERMode() && hasEMBER32r6();
//   }
//   bool hasDSP() const { return HasDSP; }
//   bool hasDSPR2() const { return HasDSPR2; }
//   bool hasDSPR3() const { return HasDSPR3; }
//   bool has3D() const { return Has3D; }
//   bool hasMSA() const { return HasMSA; }
//   bool disableMadd4() const { return DisableMadd4; }
//   bool hasEVA() const { return HasEVA; }
//   bool hasMT() const { return HasMT; }
//   bool hasCRC() const { return HasCRC; }
//   bool hasVirt() const { return HasVirt; }
//   bool hasGINV() const { return HasGINV; }
//   bool useIndirectJumpsHazard() const {
//     return UseIndirectJumpsHazard && hasEMBER32r2();
//   }
//   bool useSmallSection() const { return UseSmallSection; }
// 
//   bool hasStandardEncoding() const { return !InEMBER16Mode && !InMicroEMBERMode; }
// 
//   bool useSoftFloat() const { return IsSoftFloat; }
// 
//   bool useLongCalls() const { return UseLongCalls; }
// 
//   bool useXGOT() const { return UseXGOT; }
// 
//   bool enableLongBranchPass() const {
//     return hasStandardEncoding() || inMicroEMBERMode() || allowMixed16_32();
//   }
// 
//   /// Features related to the presence of specific instructions.
//   bool hasExtractInsert() const { return !inEMBER16Mode() && hasEMBER32r2(); }
//   bool hasMTHC1() const { return hasEMBER32r2(); }
// 
//   bool allowMixed16_32() const { return inEMBER16ModeDefault() |
//                                         AllowMixed16_32; }
// 
//   bool os16() const { return Os16; }
// 
//   bool isTargetNaCl() const { return TargetTriple.isOSNaCl(); }
// 
//   bool isXRaySupported() const override { return true; }

  // for now constant islands are on for the whole compilation unit but we only
  // really use them if in addition we are in EMBER16 mode
  static bool useConstantIslands();

  Align getStackAlignment() const { return stackAlignment; }

  // Grab relocation model
  Reloc::Model getRelocationModel() const;

  EMBERSubtarget &initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                                 const TargetMachine &TM);

  /// Does the system support unaligned memory access.
  ///
  /// EMBER32r6/EMBER64r6 require full unaligned access support but does not
  /// specify which component of the system provides it. Hardware, software, and
  /// hybrid implementations are all valid.
//   bool systemSupportsUnalignedAccess() const { return hasEMBER32r6(); }

  // Set helper classes
  void setHelperClassesEMBER16();
  void setHelperClassesEMBERSE();

  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return &TSInfo;
  }
  const EMBERInstrInfo *getInstrInfo() const override { return InstrInfo.get(); }
  const TargetFrameLowering *getFrameLowering() const override {
    return FrameLowering.get();
  }
  const EMBERRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo->getRegisterInfo();
  }
//   const EMBERTargetLowering *getTargetLowering() const override {
//     return TLInfo.get();
//   }
  const InstrItineraryData *getInstrItineraryData() const override {
    return &InstrItins;
  }

protected:
  // GlobalISel related APIs.
  std::unique_ptr<CallLowering> CallLoweringInfo;
  std::unique_ptr<LegalizerInfo> Legalizer;
  std::unique_ptr<RegisterBankInfo> RegBankInfo;
  std::unique_ptr<InstructionSelector> InstSelector;

public:
  const CallLowering *getCallLowering() const override;
  const LegalizerInfo *getLegalizerInfo() const override;
  const RegisterBankInfo *getRegBankInfo() const override;
  InstructionSelector *getInstructionSelector() const override;
};
} // End llvm namespace

#endif
