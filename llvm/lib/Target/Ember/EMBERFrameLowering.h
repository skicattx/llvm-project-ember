//===-- EMBERFrameLowering.h - Define frame lowering for EMBER ----*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_EMBERFRAMELOWERING_H
#define LLVM_LIB_TARGET_EMBER_EMBERFRAMELOWERING_H

#include "EMBER.h"
#include "llvm/CodeGen/TargetFrameLowering.h"

namespace llvm 
{
  class EMBERSubtarget;

class EMBERFrameLowering : public TargetFrameLowering 
{
protected:
  const EMBERSubtarget &STI;

public:
  explicit EMBERFrameLowering(const EMBERSubtarget &sti, Align Alignment)
      : TargetFrameLowering(StackGrowsDown, Alignment, 0, Alignment), STI(sti) {
  }

  static const EMBERFrameLowering *create(const EMBERSubtarget &ST);

  bool hasFP(const MachineFunction &MF) const override;

  bool hasBP(const MachineFunction &MF) const;

  bool isFPCloseToIncomingSP() const override { return false; }

  bool enableShrinkWrapping(const MachineFunction &MF) const override {
    return true;
  }

  MachineBasicBlock::iterator
  eliminateCallFramePseudoInstr(MachineFunction &MF,
                                MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator I) const override;

protected:
  uint64_t estimateStackSize(const MachineFunction &MF) const;
};

} // End llvm namespace

#endif
