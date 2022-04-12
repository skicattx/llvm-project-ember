//======================================================================================================================
//
// This file contains the EMBER implementation of the TargetRegisterInfo class.
// Copyright (c) 2022 IARI Ventures, LLC. All rights reserved.
//
// NOTE: A lot of the stubs and overloads in this class are for future HLL support
//======================================================================================================================


#include "EMBERRegisterInfo.h"
#include "EMBER.h"
#include "EMBERSubtarget.h"
#include "EMBERTargetMachine.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>



//#include "SparcMachineFunctionInfo.h"
//#include "llvm/CodeGen/MachineInstrBuilder.h"
//#include "llvm/CodeGen/TargetInstrInfo.h"
//#include "llvm/IR/Type.h"
//#include "llvm/Support/CommandLine.h"




using namespace llvm;

#define DEBUG_TYPE "EMBER-reg-info"

#define GET_REGINFO_TARGET_DESC
#include "EMBERGenRegisterInfo.inc"

EMBERRegisterInfo::EMBERRegisterInfo() : EMBERGenRegisterInfo(EMBER::R0) {}

const TargetRegisterClass *
EMBERRegisterInfo::getPointerRegClass(const MachineFunction &MF,
                                     unsigned Kind) const {
    return &EMBER::GPR32RegClass;
}

unsigned
EMBERRegisterInfo::getRegPressureLimit(const TargetRegisterClass *RC,
                                      MachineFunction &MF) const {
  switch (RC->getID()) 
  {
      default:
        return 0;
      case EMBER::GPR32RegClassID:
        const TargetFrameLowering *TFI = MF.getSubtarget().getFrameLowering();
        return 28 - TFI->hasFP(MF);
      }
}

//===----------------------------------------------------------------------===//
// Callee Saved Registers methods
//===----------------------------------------------------------------------===//

/// EMBER Callee Saved Registers
const MCPhysReg *
EMBERRegisterInfo::getCalleeSavedRegs(const MachineFunction *MF) const 
{
//  const EMBERSubtarget &Subtarget = MF->getSubtarget<EMBERSubtarget>();
//  const Function &F = MF->getFunction();

  // TODO: need to create this list of saved reg...calling convention
  return 0;//GPR32;
}

const uint32_t *
EMBERRegisterInfo::getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID) const {
//   const EMBERSubtarget &Subtarget = MF.getSubtarget<EMBERSubtarget>();

  // TODO: need to create this list of pres  reg...calling convention
  return 0;//::GPR32;
}

BitVector EMBERRegisterInfo::
getReservedRegs(const MachineFunction &MF) const {
  static const MCPhysReg ReservedGPR32[] = { 
      EMBER::CC, EMBER::SP
  };

  BitVector Reserved(getNumRegs());
//  const EMBERSubtarget &Subtarget = MF.getSubtarget<EMBERSubtarget>();

  for (unsigned I = 0; I < array_lengthof(ReservedGPR32); ++I)
    Reserved.set(ReservedGPR32[I]);

  return Reserved;
}

bool
EMBERRegisterInfo::requiresRegisterScavenging(const MachineFunction &MF) const {
  return true;
}

// FrameIndex represent objects inside a abstract stack.
// We must replace FrameIndex with an stack/frame pointer
// direct reference.
void EMBERRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator II, int SPAdj,
                    unsigned FIOperandNum, RegScavenger *RS) const {
  MachineInstr &MI = *II;
  MachineFunction &MF = *MI.getParent()->getParent();

  LLVM_DEBUG(errs() << "\nFunction : " << MF.getName() << "\n";
             errs() << "<--------->\n"
                    << MI);

  int FrameIndex = MI.getOperand(FIOperandNum).getIndex();
  uint64_t stackSize = MF.getFrameInfo().getStackSize();
  int64_t spOffset = MF.getFrameInfo().getObjectOffset(FrameIndex);

  LLVM_DEBUG(errs() << "FrameIndex : " << FrameIndex << "\n"
                    << "spOffset   : " << spOffset << "\n"
                    << "stackSize  : " << stackSize << "\n"
                    << "alignment  : "
                    << DebugStr(MF.getFrameInfo().getObjectAlign(FrameIndex))
                    << "\n");

  //  eliminateFI(MI, FIOperandNum, FrameIndex, stackSize, spOffset);
}

Register EMBERRegisterInfo::
getFrameRegister(const MachineFunction &MF) const {
//  const EMBERSubtarget &Subtarget = MF.getSubtarget<EMBERSubtarget>();
//  const TargetFrameLowering *TFI = Subtarget.getFrameLowering();
    return EMBER::LR;
}

bool EMBERRegisterInfo::canRealignStack(const MachineFunction &MF) const {
  // Avoid realigning functions that explicitly do not want to be realigned.
  // Normally, we should report an error when a function should be dynamically
  // realigned but also has the attribute no-realign-stack. Unfortunately,
  // with this attribute, MachineFrameInfo clamps each new object's alignment
  // to that of the stack's alignment as specified by the ABI. As a result,
  // the information of whether we have objects with larger alignment
  // requirement than the stack's alignment is already lost at this point.
  if (!TargetRegisterInfo::canRealignStack(MF))
    return false;

  return false;
}
