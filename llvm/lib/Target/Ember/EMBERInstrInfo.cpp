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

EMBERInstrInfo::EMBERInstrInfo(const EMBERSubtarget &STI, unsigned UncondBr)
    : EMBERGenInstrInfo(0/*EMBER::ADJCALLSTACKDOWN*/, 0/*EMBER::ADJCALLSTACKUP*/),
      Subtarget(STI), UncondBrOpc(UncondBr) {}

const EMBERInstrInfo *EMBERInstrInfo::create(EMBERSubtarget &STI) {

  return createEMBERSEInstrInfo(STI);
}
/*
bool EMBERInstrInfo::isZeroImm(const MachineOperand &op) const {
  return op.isImm() && op.getImm() == 0;
}

/// insertNoop - If data hazard condition is found insert the target nop
/// instruction.
// FIXME: This appears to be dead code.
void EMBERInstrInfo::
insertNoop(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI) const
{
  DebugLoc DL;
  BuildMI(MBB, MI, DL, get(EMBER::NOP));
}

MachineMemOperand *
EMBERInstrInfo::GetMemOperand(MachineBasicBlock &MBB, int FI,
                             MachineMemOperand::Flags Flags) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo &MFI = MF.getFrameInfo();

  return MF.getMachineMemOperand(MachinePointerInfo::getFixedStack(MF, FI),
                                 Flags, MFI.getObjectSize(FI),
                                 MFI.getObjectAlign(FI));
}

//===----------------------------------------------------------------------===//
// Branch Analysis
//===----------------------------------------------------------------------===//

void EMBERInstrInfo::AnalyzeCondBr(const MachineInstr *Inst, unsigned Opc,
                                  MachineBasicBlock *&BB,
                                  SmallVectorImpl<MachineOperand> &Cond) const {
  assert(getAnalyzableBrOpc(Opc) && "Not an analyzable branch");
  int NumOp = Inst->getNumExplicitOperands();

  // for both int and fp branches, the last explicit operand is the
  // MBB.
  BB = Inst->getOperand(NumOp-1).getMBB();
  Cond.push_back(MachineOperand::CreateImm(Opc));

  for (int i = 0; i < NumOp-1; i++)
    Cond.push_back(Inst->getOperand(i));
}

bool EMBERInstrInfo::analyzeBranch(MachineBasicBlock &MBB,
                                  MachineBasicBlock *&TBB,
                                  MachineBasicBlock *&FBB,
                                  SmallVectorImpl<MachineOperand> &Cond,
                                  bool AllowModify) const {
  SmallVector<MachineInstr*, 2> BranchInstrs;
  BranchType BT = analyzeBranch(MBB, TBB, FBB, Cond, AllowModify, BranchInstrs);

  return (BT == BT_None) || (BT == BT_Indirect);
}

void EMBERInstrInfo::BuildCondBr(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                                const DebugLoc &DL,
                                ArrayRef<MachineOperand> Cond) const {
  unsigned Opc = Cond[0].getImm();
  const MCInstrDesc &MCID = get(Opc);
  MachineInstrBuilder MIB = BuildMI(&MBB, DL, MCID);

  for (unsigned i = 1; i < Cond.size(); ++i) {
    assert((Cond[i].isImm() || Cond[i].isReg()) &&
           "Cannot copy operand for conditional branch!");
    MIB.add(Cond[i]);
  }
  MIB.addMBB(TBB);
}

unsigned EMBERInstrInfo::insertBranch(MachineBasicBlock &MBB,
                                     MachineBasicBlock *TBB,
                                     MachineBasicBlock *FBB,
                                     ArrayRef<MachineOperand> Cond,
                                     const DebugLoc &DL,
                                     int *BytesAdded) const {
  // Shouldn't be a fall through.
  assert(TBB && "insertBranch must not be told to insert a fallthrough");
  assert(!BytesAdded && "code size not handled");

  // # of condition operands:
  //  Unconditional branches: 0
  //  Floating point branches: 1 (opc)
  //  Int BranchZero: 2 (opc, reg)
  //  Int Branch: 3 (opc, reg0, reg1)
  assert((Cond.size() <= 3) &&
         "# of EMBER branch conditions must be <= 3!");

  // Two-way Conditional branch.
  if (FBB) {
    BuildCondBr(MBB, TBB, DL, Cond);
    BuildMI(&MBB, DL, get(UncondBrOpc)).addMBB(FBB);
    return 2;
  }

  // One way branch.
  // Unconditional branch.
  if (Cond.empty())
    BuildMI(&MBB, DL, get(UncondBrOpc)).addMBB(TBB);
  else // Conditional branch.
    BuildCondBr(MBB, TBB, DL, Cond);
  return 1;
}

unsigned EMBERInstrInfo::removeBranch(MachineBasicBlock &MBB,
                                     int *BytesRemoved) const {
  assert(!BytesRemoved && "code size not handled");

  MachineBasicBlock::reverse_iterator I = MBB.rbegin(), REnd = MBB.rend();
  unsigned removed = 0;

  // Up to 2 branches are removed.
  // Note that indirect branches are not removed.
  while (I != REnd && removed < 2) {
    // Skip past debug instructions.
    if (I->isDebugInstr()) {
      ++I;
      continue;
    }
    if (!getAnalyzableBrOpc(I->getOpcode()))
      break;
    // Remove the branch.
    I->eraseFromParent();
    I = MBB.rbegin();
    ++removed;
  }

  return removed;
}

/// reverseBranchCondition - Return the inverse opcode of the
/// specified Branch instruction.
bool EMBERInstrInfo::reverseBranchCondition(
    SmallVectorImpl<MachineOperand> &Cond) const {
  assert( (Cond.size() && Cond.size() <= 3) &&
          "Invalid EMBER branch condition!");
  Cond[0].setImm(getOppositeBranchOpc(Cond[0].getImm()));
  return false;
}

EMBERInstrInfo::BranchType EMBERInstrInfo::analyzeBranch(
    MachineBasicBlock &MBB, MachineBasicBlock *&TBB, MachineBasicBlock *&FBB,
    SmallVectorImpl<MachineOperand> &Cond, bool AllowModify,
    SmallVectorImpl<MachineInstr *> &BranchInstrs) const {
  MachineBasicBlock::reverse_iterator I = MBB.rbegin(), REnd = MBB.rend();

  // Skip all the debug instructions.
  while (I != REnd && I->isDebugInstr())
    ++I;

  if (I == REnd || !isUnpredicatedTerminator(*I)) {
    // This block ends with no branches (it just falls through to its succ).
    // Leave TBB/FBB null.
    TBB = FBB = nullptr;
    return BT_NoBranch;
  }

  MachineInstr *LastInst = &*I;
  unsigned LastOpc = LastInst->getOpcode();
  BranchInstrs.push_back(LastInst);

  // Not an analyzable branch (e.g., indirect jump).
  if (!getAnalyzableBrOpc(LastOpc))
    return LastInst->isIndirectBranch() ? BT_Indirect : BT_None;

  // Get the second to last instruction in the block.
  unsigned SecondLastOpc = 0;
  MachineInstr *SecondLastInst = nullptr;

  // Skip past any debug instruction to see if the second last actual
  // is a branch.
  ++I;
  while (I != REnd && I->isDebugInstr())
    ++I;

  if (I != REnd) {
    SecondLastInst = &*I;
    SecondLastOpc = getAnalyzableBrOpc(SecondLastInst->getOpcode());

    // Not an analyzable branch (must be an indirect jump).
    if (isUnpredicatedTerminator(*SecondLastInst) && !SecondLastOpc)
      return BT_None;
  }

  // If there is only one terminator instruction, process it.
  if (!SecondLastOpc) {
    // Unconditional branch.
    if (LastInst->isUnconditionalBranch()) {
      TBB = LastInst->getOperand(0).getMBB();
      return BT_Uncond;
    }

    // Conditional branch
    AnalyzeCondBr(LastInst, LastOpc, TBB, Cond);
    return BT_Cond;
  }

  // If we reached here, there are two branches.
  // If there are three terminators, we don't know what sort of block this is.
  if (++I != REnd && isUnpredicatedTerminator(*I))
    return BT_None;

  BranchInstrs.insert(BranchInstrs.begin(), SecondLastInst);

  // If second to last instruction is an unconditional branch,
  // analyze it and remove the last instruction.
  if (SecondLastInst->isUnconditionalBranch()) {
    // Return if the last instruction cannot be removed.
    if (!AllowModify)
      return BT_None;

    TBB = SecondLastInst->getOperand(0).getMBB();
    LastInst->eraseFromParent();
    BranchInstrs.pop_back();
    return BT_Uncond;
  }

  // Conditional branch followed by an unconditional branch.
  // The last one must be unconditional.
  if (!LastInst->isUnconditionalBranch())
    return BT_None;

  AnalyzeCondBr(SecondLastInst, SecondLastOpc, TBB, Cond);
  FBB = LastInst->getOperand(0).getMBB();

  return BT_CondUncond;
}

bool EMBERInstrInfo::isBranchOffsetInRange(unsigned BranchOpc,
                                          int64_t BrOffset) const {
  switch (BranchOpc) {
  case EMBER::B:
  case EMBER::BAL:
  case EMBER::BAL_BR:
  case EMBER::BAL_BR_MM:
  case EMBER::BC1F:
  case EMBER::BC1FL:
  case EMBER::BC1T:
  case EMBER::BC1TL:
  case EMBER::BEQ:     case EMBER::BEQ64:
  case EMBER::BEQL:
  case EMBER::BGEZ:    case EMBER::BGEZ64:
  case EMBER::BGEZL:
  case EMBER::BGEZAL:
  case EMBER::BGEZALL:
  case EMBER::BGTZ:    case EMBER::BGTZ64:
  case EMBER::BGTZL:
  case EMBER::BLEZ:    case EMBER::BLEZ64:
  case EMBER::BLEZL:
  case EMBER::BLTZ:    case EMBER::BLTZ64:
  case EMBER::BLTZL:
  case EMBER::BLTZAL:
  case EMBER::BLTZALL:
  case EMBER::BNE:     case EMBER::BNE64:
  case EMBER::BNEL:
    return isInt<18>(BrOffset);

  // microEMBERr3 branches
  case EMBER::B_MM:
  case EMBER::BC1F_MM:
  case EMBER::BC1T_MM:
  case EMBER::BEQ_MM:
  case EMBER::BGEZ_MM:
  case EMBER::BGEZAL_MM:
  case EMBER::BGTZ_MM:
  case EMBER::BLEZ_MM:
  case EMBER::BLTZ_MM:
  case EMBER::BLTZAL_MM:
  case EMBER::BNE_MM:
  case EMBER::BEQZC_MM:
  case EMBER::BNEZC_MM:
    return isInt<17>(BrOffset);

  // microEMBERR3 short branches.
  case EMBER::B16_MM:
    return isInt<11>(BrOffset);

  case EMBER::BEQZ16_MM:
  case EMBER::BNEZ16_MM:
    return isInt<8>(BrOffset);

  // EMBERR6 branches.
  case EMBER::BALC:
  case EMBER::BC:
    return isInt<28>(BrOffset);

  case EMBER::BC1EQZ:
  case EMBER::BC1NEZ:
  case EMBER::BC2EQZ:
  case EMBER::BC2NEZ:
  case EMBER::BEQC:   case EMBER::BEQC64:
  case EMBER::BNEC:   case EMBER::BNEC64:
  case EMBER::BGEC:   case EMBER::BGEC64:
  case EMBER::BGEUC:  case EMBER::BGEUC64:
  case EMBER::BGEZC:  case EMBER::BGEZC64:
  case EMBER::BGTZC:  case EMBER::BGTZC64:
  case EMBER::BLEZC:  case EMBER::BLEZC64:
  case EMBER::BLTC:   case EMBER::BLTC64:
  case EMBER::BLTUC:  case EMBER::BLTUC64:
  case EMBER::BLTZC:  case EMBER::BLTZC64:
  case EMBER::BNVC:
  case EMBER::BOVC:
  case EMBER::BGEZALC:
  case EMBER::BEQZALC:
  case EMBER::BGTZALC:
  case EMBER::BLEZALC:
  case EMBER::BLTZALC:
  case EMBER::BNEZALC:
    return isInt<18>(BrOffset);

  case EMBER::BEQZC:  case EMBER::BEQZC64:
  case EMBER::BNEZC:  case EMBER::BNEZC64:
    return isInt<23>(BrOffset);

  // microEMBERR6 branches
  case EMBER::BC16_MMR6:
    return isInt<11>(BrOffset);

  case EMBER::BEQZC16_MMR6:
  case EMBER::BNEZC16_MMR6:
    return isInt<8>(BrOffset);

  case EMBER::BALC_MMR6:
  case EMBER::BC_MMR6:
    return isInt<27>(BrOffset);

  case EMBER::BC1EQZC_MMR6:
  case EMBER::BC1NEZC_MMR6:
  case EMBER::BC2EQZC_MMR6:
  case EMBER::BC2NEZC_MMR6:
  case EMBER::BGEZALC_MMR6:
  case EMBER::BEQZALC_MMR6:
  case EMBER::BGTZALC_MMR6:
  case EMBER::BLEZALC_MMR6:
  case EMBER::BLTZALC_MMR6:
  case EMBER::BNEZALC_MMR6:
  case EMBER::BNVC_MMR6:
  case EMBER::BOVC_MMR6:
    return isInt<17>(BrOffset);

  case EMBER::BEQC_MMR6:
  case EMBER::BNEC_MMR6:
  case EMBER::BGEC_MMR6:
  case EMBER::BGEUC_MMR6:
  case EMBER::BGEZC_MMR6:
  case EMBER::BGTZC_MMR6:
  case EMBER::BLEZC_MMR6:
  case EMBER::BLTC_MMR6:
  case EMBER::BLTUC_MMR6:
  case EMBER::BLTZC_MMR6:
    return isInt<18>(BrOffset);

  case EMBER::BEQZC_MMR6:
  case EMBER::BNEZC_MMR6:
    return isInt<23>(BrOffset);

  // DSP branches.
  case EMBER::BPOSGE32:
    return isInt<18>(BrOffset);
  case EMBER::BPOSGE32_MM:
  case EMBER::BPOSGE32C_MMR3:
    return isInt<17>(BrOffset);

  // cnEMBER branches.
  case EMBER::BBIT0:
  case EMBER::BBIT032:
  case EMBER::BBIT1:
  case EMBER::BBIT132:
    return isInt<18>(BrOffset);

  // MSA branches.
  case EMBER::BZ_B:
  case EMBER::BZ_H:
  case EMBER::BZ_W:
  case EMBER::BZ_D:
  case EMBER::BZ_V:
  case EMBER::BNZ_B:
  case EMBER::BNZ_H:
  case EMBER::BNZ_W:
  case EMBER::BNZ_D:
  case EMBER::BNZ_V:
    return isInt<18>(BrOffset);
  }

  llvm_unreachable("Unknown branch instruction!");
}

/// Return the corresponding compact (no delay slot) form of a branch.
unsigned EMBERInstrInfo::getEquivalentCompactForm(
    const MachineBasicBlock::iterator I) const {
  unsigned Opcode = I->getOpcode();
  bool canUseShortMicroEMBERCTI = false;

  if (Subtarget.inMicroEMBERMode()) {
    switch (Opcode) {
    case EMBER::BNE:
    case EMBER::BNE_MM:
    case EMBER::BEQ:
    case EMBER::BEQ_MM:
    // microEMBER has NE,EQ branches that do not have delay slots provided one
    // of the operands is zero.
      if (I->getOperand(1).getReg() == Subtarget.getABI().GetZeroReg())
        canUseShortMicroEMBERCTI = true;
      break;
    // For microEMBER the PseudoReturn and PseudoIndirectBranch are always
    // expanded to JR_MM, so they can be replaced with JRC16_MM.
    case EMBER::JR:
    case EMBER::PseudoReturn:
    case EMBER::PseudoIndirectBranch:
      canUseShortMicroEMBERCTI = true;
      break;
    }
  }

  // EMBERR6 forbids both operands being the zero register.
  if (Subtarget.hasEMBER32r6() && (I->getNumOperands() > 1) &&
      (I->getOperand(0).isReg() &&
       (I->getOperand(0).getReg() == EMBER::ZERO ||
        I->getOperand(0).getReg() == EMBER::ZERO_64)) &&
      (I->getOperand(1).isReg() &&
       (I->getOperand(1).getReg() == EMBER::ZERO ||
        I->getOperand(1).getReg() == EMBER::ZERO_64)))
    return 0;

  if (Subtarget.hasEMBER32r6() || canUseShortMicroEMBERCTI) {
    switch (Opcode) {
    case EMBER::B:
      return EMBER::BC;
    case EMBER::BAL:
      return EMBER::BALC;
    case EMBER::BEQ:
    case EMBER::BEQ_MM:
      if (canUseShortMicroEMBERCTI)
        return EMBER::BEQZC_MM;
      else if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BEQC;
    case EMBER::BNE:
    case EMBER::BNE_MM:
      if (canUseShortMicroEMBERCTI)
        return EMBER::BNEZC_MM;
      else if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BNEC;
    case EMBER::BGE:
      if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BGEC;
    case EMBER::BGEU:
      if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BGEUC;
    case EMBER::BGEZ:
      return EMBER::BGEZC;
    case EMBER::BGTZ:
      return EMBER::BGTZC;
    case EMBER::BLEZ:
      return EMBER::BLEZC;
    case EMBER::BLT:
      if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BLTC;
    case EMBER::BLTU:
      if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BLTUC;
    case EMBER::BLTZ:
      return EMBER::BLTZC;
    case EMBER::BEQ64:
      if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BEQC64;
    case EMBER::BNE64:
      if (I->getOperand(0).getReg() == I->getOperand(1).getReg())
        return 0;
      return EMBER::BNEC64;
    case EMBER::BGTZ64:
      return EMBER::BGTZC64;
    case EMBER::BGEZ64:
      return EMBER::BGEZC64;
    case EMBER::BLTZ64:
      return EMBER::BLTZC64;
    case EMBER::BLEZ64:
      return EMBER::BLEZC64;
    // For EMBERR6, the instruction 'jic' can be used for these cases. Some
    // tools will accept 'jrc reg' as an alias for 'jic 0, $reg'.
    case EMBER::JR:
    case EMBER::PseudoIndirectBranchR6:
    case EMBER::PseudoReturn:
    case EMBER::TAILCALLR6REG:
      if (canUseShortMicroEMBERCTI)
        return EMBER::JRC16_MM;
      return EMBER::JIC;
    case EMBER::JALRPseudo:
      return EMBER::JIALC;
    case EMBER::JR64:
    case EMBER::PseudoIndirectBranch64R6:
    case EMBER::PseudoReturn64:
    case EMBER::TAILCALL64R6REG:
      return EMBER::JIC64;
    case EMBER::JALR64Pseudo:
      return EMBER::JIALC64;
    default:
      return 0;
    }
  }

  return 0;
}

/// Predicate for distingushing between control transfer instructions and all
/// other instructions for handling forbidden slots. Consider inline assembly
/// as unsafe as well.
bool EMBERInstrInfo::SafeInForbiddenSlot(const MachineInstr &MI) const {
  if (MI.isInlineAsm())
    return false;

  return (MI.getDesc().TSFlags & EMBERII::IsCTI) == 0;
}

/// Predicate for distingushing instructions that have forbidden slots.
bool EMBERInstrInfo::HasForbiddenSlot(const MachineInstr &MI) const {
  return (MI.getDesc().TSFlags & EMBERII::HasForbiddenSlot) != 0;
}

/// Return the number of bytes of code the specified instruction may be.
unsigned EMBERInstrInfo::getInstSizeInBytes(const MachineInstr &MI) const {
  switch (MI.getOpcode()) {
  default:
    return MI.getDesc().getSize();
  case  TargetOpcode::INLINEASM:
  case  TargetOpcode::INLINEASM_BR: {       // Inline Asm: Variable size.
    const MachineFunction *MF = MI.getParent()->getParent();
    const char *AsmStr = MI.getOperand(0).getSymbolName();
    return getInlineAsmLength(AsmStr, *MF->getTarget().getMCAsmInfo());
  }
  case EMBER::CONSTPOOL_ENTRY:
    // If this machine instr is a constant pool entry, its size is recorded as
    // operand #2.
    return MI.getOperand(2).getImm();
  }
}

MachineInstrBuilder
EMBERInstrInfo::genInstrWithNewOpc(unsigned NewOpc,
                                  MachineBasicBlock::iterator I) const {
  MachineInstrBuilder MIB;

  // Certain branches have two forms: e.g beq $1, $zero, dest vs beqz $1, dest
  // Pick the zero form of the branch for readable assembly and for greater
  // branch distance in non-microEMBER mode.
  // Additional EMBERR6 does not permit the use of register $zero for compact
  // branches.
  // FIXME: Certain atomic sequences on EMBER64 generate 32bit references to
  // EMBER::ZERO, which is incorrect. This test should be updated to use
  // Subtarget.getABI().GetZeroReg() when those atomic sequences and others
  // are fixed.
  int ZeroOperandPosition = -1;
  bool BranchWithZeroOperand = false;
  if (I->isBranch() && !I->isPseudo()) {
    auto TRI = I->getParent()->getParent()->getSubtarget().getRegisterInfo();
    ZeroOperandPosition = I->findRegisterUseOperandIdx(EMBER::ZERO, false, TRI);
    BranchWithZeroOperand = ZeroOperandPosition != -1;
  }

  if (BranchWithZeroOperand) {
    switch (NewOpc) {
    case EMBER::BEQC:
      NewOpc = EMBER::BEQZC;
      break;
    case EMBER::BNEC:
      NewOpc = EMBER::BNEZC;
      break;
    case EMBER::BGEC:
      NewOpc = EMBER::BGEZC;
      break;
    case EMBER::BLTC:
      NewOpc = EMBER::BLTZC;
      break;
    case EMBER::BEQC64:
      NewOpc = EMBER::BEQZC64;
      break;
    case EMBER::BNEC64:
      NewOpc = EMBER::BNEZC64;
      break;
    }
  }

  MIB = BuildMI(*I->getParent(), I, I->getDebugLoc(), get(NewOpc));

  // For EMBERR6 JI*C requires an immediate 0 as an operand, JIALC(64) an
  // immediate 0 as an operand and requires the removal of it's implicit-def %ra
  // implicit operand as copying the implicit operations of the instructio we're
  // looking at will give us the correct flags.
  if (NewOpc == EMBER::JIC || NewOpc == EMBER::JIALC || NewOpc == EMBER::JIC64 ||
      NewOpc == EMBER::JIALC64) {

    if (NewOpc == EMBER::JIALC || NewOpc == EMBER::JIALC64)
      MIB->RemoveOperand(0);

    for (unsigned J = 0, E = I->getDesc().getNumOperands(); J < E; ++J) {
      MIB.add(I->getOperand(J));
    }

    MIB.addImm(0);

    // If I has an MCSymbol operand (used by asm printer, to emit R_EMBER_JALR),
    // add it to the new instruction.
    for (unsigned J = I->getDesc().getNumOperands(), E = I->getNumOperands();
         J < E; ++J) {
      const MachineOperand &MO = I->getOperand(J);
      if (MO.isMCSymbol() && (MO.getTargetFlags() & EMBERII::MO_JALR))
        MIB.addSym(MO.getMCSymbol(), EMBERII::MO_JALR);
    }


  } else {
    for (unsigned J = 0, E = I->getDesc().getNumOperands(); J < E; ++J) {
      if (BranchWithZeroOperand && (unsigned)ZeroOperandPosition == J)
        continue;

      MIB.add(I->getOperand(J));
    }
  }

  MIB.copyImplicitOps(*I);
  MIB.cloneMemRefs(*I);
  return MIB;
}

bool EMBERInstrInfo::findCommutedOpIndices(const MachineInstr &MI,
                                          unsigned &SrcOpIdx1,
                                          unsigned &SrcOpIdx2) const {
  assert(!MI.isBundle() &&
         "TargetInstrInfo::findCommutedOpIndices() can't handle bundles");

  const MCInstrDesc &MCID = MI.getDesc();
  if (!MCID.isCommutable())
    return false;

  switch (MI.getOpcode()) {
  case EMBER::DPADD_U_H:
  case EMBER::DPADD_U_W:
  case EMBER::DPADD_U_D:
  case EMBER::DPADD_S_H:
  case EMBER::DPADD_S_W:
  case EMBER::DPADD_S_D:
    // The first operand is both input and output, so it should not commute
    if (!fixCommutedOpIndices(SrcOpIdx1, SrcOpIdx2, 2, 3))
      return false;

    if (!MI.getOperand(SrcOpIdx1).isReg() || !MI.getOperand(SrcOpIdx2).isReg())
      return false;
    return true;
  }
  return TargetInstrInfo::findCommutedOpIndices(MI, SrcOpIdx1, SrcOpIdx2);
}

// ins, ext, dext*, dins have the following constraints:
// X <= pos      <  Y
// X <  size     <= Y
// X <  pos+size <= Y
//
// dinsm and dinsu have the following constraints:
// X <= pos      <  Y
// X <= size     <= Y
// X <  pos+size <= Y
//
// The callee of verifyInsExtInstruction however gives the bounds of
// dins[um] like the other (d)ins (d)ext(um) instructions, so that this
// function doesn't have to vary it's behaviour based on the instruction
// being checked.
static bool verifyInsExtInstruction(const MachineInstr &MI, StringRef &ErrInfo,
                                    const int64_t PosLow, const int64_t PosHigh,
                                    const int64_t SizeLow,
                                    const int64_t SizeHigh,
                                    const int64_t BothLow,
                                    const int64_t BothHigh) {
  MachineOperand MOPos = MI.getOperand(2);
  if (!MOPos.isImm()) {
    ErrInfo = "Position is not an immediate!";
    return false;
  }
  int64_t Pos = MOPos.getImm();
  if (!((PosLow <= Pos) && (Pos < PosHigh))) {
    ErrInfo = "Position operand is out of range!";
    return false;
  }

  MachineOperand MOSize = MI.getOperand(3);
  if (!MOSize.isImm()) {
    ErrInfo = "Size operand is not an immediate!";
    return false;
  }
  int64_t Size = MOSize.getImm();
  if (!((SizeLow < Size) && (Size <= SizeHigh))) {
    ErrInfo = "Size operand is out of range!";
    return false;
  }

  if (!((BothLow < (Pos + Size)) && ((Pos + Size) <= BothHigh))) {
    ErrInfo = "Position + Size is out of range!";
    return false;
  }

  return true;
}

//  Perform target specific instruction verification.
bool EMBERInstrInfo::verifyInstruction(const MachineInstr &MI,
                                      StringRef &ErrInfo) const {
  // Verify that ins and ext instructions are well formed.
  switch (MI.getOpcode()) {
    case EMBER::EXT:
    case EMBER::EXT_MM:
    case EMBER::INS:
    case EMBER::INS_MM:
    case EMBER::DINS:
      return verifyInsExtInstruction(MI, ErrInfo, 0, 32, 0, 32, 0, 32);
    case EMBER::DINSM:
      // The ISA spec has a subtle difference between dinsm and dextm
      // in that it says:
      // 2 <= size <= 64 for 'dinsm' but 'dextm' has 32 < size <= 64.
      // To make the bounds checks similar, the range 1 < size <= 64 is checked
      // for 'dinsm'.
      return verifyInsExtInstruction(MI, ErrInfo, 0, 32, 1, 64, 32, 64);
    case EMBER::DINSU:
      // The ISA spec has a subtle difference between dinsu and dextu in that
      // the size range of dinsu is specified as 1 <= size <= 32 whereas size
      // for dextu is 0 < size <= 32. The range checked for dinsu here is
      // 0 < size <= 32, which is equivalent and similar to dextu.
      return verifyInsExtInstruction(MI, ErrInfo, 32, 64, 0, 32, 32, 64);
    case EMBER::DEXT:
      return verifyInsExtInstruction(MI, ErrInfo, 0, 32, 0, 32, 0, 63);
    case EMBER::DEXTM:
      return verifyInsExtInstruction(MI, ErrInfo, 0, 32, 32, 64, 32, 64);
    case EMBER::DEXTU:
      return verifyInsExtInstruction(MI, ErrInfo, 32, 64, 0, 32, 32, 64);
    case EMBER::TAILCALLREG:
    case EMBER::PseudoIndirectBranch:
    case EMBER::JR:
    case EMBER::JR64:
    case EMBER::JALR:
    case EMBER::JALR64:
    case EMBER::JALRPseudo:
      if (!Subtarget.useIndirectJumpsHazard())
        return true;

      ErrInfo = "invalid instruction when using jump guards!";
      return false;
    default:
      return true;
  }

  return true;
}

std::pair<unsigned, unsigned>
EMBERInstrInfo::decomposeMachineOperandsTargetFlags(unsigned TF) const {
  return std::make_pair(TF, 0u);
}

ArrayRef<std::pair<unsigned, const char*>>
EMBERInstrInfo::getSerializableDirectMachineOperandTargetFlags() const {
 using namespace EMBERII;

 static const std::pair<unsigned, const char*> Flags[] = {
    {MO_GOT,          "EMBER-got"},
    {MO_GOT_CALL,     "EMBER-got-call"},
    {MO_GPREL,        "EMBER-gprel"},
    {MO_ABS_HI,       "EMBER-abs-hi"},
    {MO_ABS_LO,       "EMBER-abs-lo"},
    {MO_TLSGD,        "EMBER-tlsgd"},
    {MO_TLSLDM,       "EMBER-tlsldm"},
    {MO_DTPREL_HI,    "EMBER-dtprel-hi"},
    {MO_DTPREL_LO,    "EMBER-dtprel-lo"},
    {MO_GOTTPREL,     "EMBER-gottprel"},
    {MO_TPREL_HI,     "EMBER-tprel-hi"},
    {MO_TPREL_LO,     "EMBER-tprel-lo"},
    {MO_GPOFF_HI,     "EMBER-gpoff-hi"},
    {MO_GPOFF_LO,     "EMBER-gpoff-lo"},
    {MO_GOT_DISP,     "EMBER-got-disp"},
    {MO_GOT_PAGE,     "EMBER-got-page"},
    {MO_GOT_OFST,     "EMBER-got-ofst"},
    {MO_HIGHER,       "EMBER-higher"},
    {MO_HIGHEST,      "EMBER-highest"},
    {MO_GOT_HI16,     "EMBER-got-hi16"},
    {MO_GOT_LO16,     "EMBER-got-lo16"},
    {MO_CALL_HI16,    "EMBER-call-hi16"},
    {MO_CALL_LO16,    "EMBER-call-lo16"},
    {MO_JALR,         "EMBER-jalr"}
  };
  return makeArrayRef(Flags);
}

Optional<ParamLoadedValue>
EMBERInstrInfo::describeLoadedValue(const MachineInstr &MI, Register Reg) const {
  DIExpression *Expr =
      DIExpression::get(MI.getMF()->getFunction().getContext(), {});

  // TODO: Special EMBER instructions that need to be described separately.
  if (auto RegImm = isAddImmediate(MI, Reg)) {
    Register SrcReg = RegImm->Reg;
    int64_t Offset = RegImm->Imm;
    // When SrcReg is $zero, treat loaded value as immediate only.
    // Ex. $a2 = ADDiu $zero, 10
    if (SrcReg == EMBER::ZERO || SrcReg == EMBER::ZERO_64) {
      return ParamLoadedValue(MI.getOperand(2), Expr);
    }
    Expr = DIExpression::prepend(Expr, DIExpression::ApplyOffset, Offset);
    return ParamLoadedValue(MachineOperand::CreateReg(SrcReg, false), Expr);
  } else if (auto DestSrc = isCopyInstr(MI)) {
    const MachineFunction *MF = MI.getMF();
    const TargetRegisterInfo *TRI = MF->getSubtarget().getRegisterInfo();
    Register DestReg = DestSrc->Destination->getReg();
    // TODO: Handle cases where the Reg is sub- or super-register of the
    // DestReg.
    if (TRI->isSuperRegister(Reg, DestReg) || TRI->isSubRegister(Reg, DestReg))
      return None;
  }

  return TargetInstrInfo::describeLoadedValue(MI, Reg);
}

Optional<RegImmPair> EMBERInstrInfo::isAddImmediate(const MachineInstr &MI,
                                                   Register Reg) const {
  // TODO: Handle cases where Reg is a super- or sub-register of the
  // destination register.
  const MachineOperand &Op0 = MI.getOperand(0);
  if (!Op0.isReg() || Reg != Op0.getReg())
    return None;

  switch (MI.getOpcode()) {
  case EMBER::ADDiu:
  case EMBER::DADDiu: {
    const MachineOperand &Dop = MI.getOperand(0);
    const MachineOperand &Sop1 = MI.getOperand(1);
    const MachineOperand &Sop2 = MI.getOperand(2);
    // Value is sum of register and immediate. Immediate value could be
    // global string address which is not supported.
    if (Dop.isReg() && Sop1.isReg() && Sop2.isImm())
      return RegImmPair{Sop1.getReg(), Sop2.getImm()};
    // TODO: Handle case where Sop1 is a frame-index.
  }
  }
  return None;
}
*/