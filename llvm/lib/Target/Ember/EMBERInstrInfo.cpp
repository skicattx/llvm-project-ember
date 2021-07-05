//===- EMBERInstrInfo.cpp - EMBER Instruction Information -------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the EMBER implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#include "EMBERInstrInfo.h"
#include "MCTargetDesc/EMBERBaseInfo.h"
#include "MCTargetDesc/EMBERMCTargetDesc.h"
#include "EMBERSubtarget.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/CodeGen/TargetOpcodes.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/Target/TargetMachine.h"
#include <cassert>

using namespace llvm;

#define GET_INSTRINFO_CTOR_DTOR
#include "EMBERGenInstrInfo.inc"

// Pin the vtable to this file.
void EMBERInstrInfo::anchor() {}

EMBERInstrInfo::EMBERInstrInfo(const EMBERSubtarget &STI, unsigned UncondBr) :
    EMBERGenInstrInfo(0/*EMBER::ADJCALLSTACKDOWN*/, 0/*EMBER::ADJCALLSTACKUP*/),
    Subtarget(STI), 
    UncondBrOpc(UncondBr)
{
}

const EMBERInstrInfo *EMBERInstrInfo::create(EMBERSubtarget &STI) 
{
    return createEMBERSEInstrInfo(STI);
}
