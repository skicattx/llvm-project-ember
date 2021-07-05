//===- EMBERInstrInfo.h - EMBER Instruction Information -----------*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the EMBER implementation of the TargetInstrInfo class.
//
// FIXME: We need to override TargetInstrInfo::getInlineAsmLength method in
// order for EMBERLongBranch pass to work correctly when the code has inline
// assembly.  The returned value doesn't have to be the asm instruction's exact
// size in bytes; EMBERLongBranch only expects it to be the correct upper bound.
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_EMBERINSTRINFO_H
#define LLVM_LIB_TARGET_EMBER_EMBERINSTRINFO_H

#include "MCTargetDesc/EMBERMCTargetDesc.h"
#include "EMBER.h"
#include "EMBERRegisterInfo.h"
#include "llvm/CodeGen/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "EMBERGenInstrInfo.inc"

namespace llvm 
{

class MachineInstr;
class MachineOperand;
class EMBERSubtarget;
class TargetRegisterClass;
class TargetRegisterInfo;

class EMBERInstrInfo : public EMBERGenInstrInfo 
{
    virtual void anchor();

protected:
    const EMBERSubtarget &Subtarget;
    unsigned UncondBrOpc;

public:
    explicit EMBERInstrInfo(const EMBERSubtarget &STI, unsigned UncondBrOpc);

    static const EMBERInstrInfo *create(EMBERSubtarget &STI);

    /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
    /// such, whenever a client has an instance of instruction info, it should
    /// always be able to get register info as well (through this method).
    virtual const EMBERRegisterInfo &getRegisterInfo() const = 0;
};

/// Create EMBERInstrInfo objects.
const EMBERInstrInfo *createEMBERSEInstrInfo(const EMBERSubtarget &STI);

} // end namespace llvm

#endif // LLVM_LIB_TARGET_EMBER_EMBERINSTRINFO_H
