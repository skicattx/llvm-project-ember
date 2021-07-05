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
#include "EMBERRegisterInfo.h"
#include "EMBERTargetMachine.h"
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
// static cl::opt<bool>
//     Mixed16_32("EMBER-mixed-16-32", cl::init(false),
//                cl::desc("Allow for a mixture of EMBER16 "
//                         "and EMBER32 code in a single output file"),
//                cl::Hidden);
// 
// static cl::opt<bool> EMBER_Os16("EMBER-os16", cl::init(false),
//                                cl::desc("Compile all functions that don't use "
//                                         "floating point as EMBER 16"),
//                                cl::Hidden);
// 
// static cl::opt<bool> EMBER16HardFloat("EMBER16-hard-float", cl::NotHidden,
//                                      cl::desc("Enable EMBER16 hard float."),
//                                      cl::init(false));
// 
// static cl::opt<bool>
//     EMBER16ConstantIslands("EMBER16-constant-islands", cl::NotHidden,
//                           cl::desc("Enable EMBER16 constant islands."),
//                           cl::init(true));
// 
// static cl::opt<bool>
//     GPOpt("mgpopt", cl::Hidden,
//           cl::desc("Enable gp-relative addressing of EMBER small data items"));

void EMBERSubtarget::anchor() {}

EMBERSubtarget::EMBERSubtarget(const Triple &TT, StringRef CPU, StringRef FS, const EMBERTargetMachine &TM) :
    EMBERGenSubtargetInfo(TT, CPU, CPU, FS), 
    TM(TM),
    TSInfo(), 
    InstrInfo(EMBERInstrInfo::create( initializeSubtargetDependencies(CPU, FS, TM)))
{
}

bool EMBERSubtarget::isPositionIndependent() const 
{
  return TM.isPositionIndependent();
}

CodeGenOpt::Level EMBERSubtarget::getOptLevelToEnablePostRAScheduler() const {
  return CodeGenOpt::Aggressive;
}

EMBERSubtarget &
EMBERSubtarget::initializeSubtargetDependencies(StringRef CPU, StringRef FS,
                                               const TargetMachine &TM) 
{
  return *this;
}

Reloc::Model EMBERSubtarget::getRelocationModel() const {
  return TM.getRelocationModel();
}


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
