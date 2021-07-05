//===-- EMBERMCCodeEmitter.cpp - Convert EMBER code to machine code -------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file implements the EMBERMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/EMBERMCTargetDesc.h"
//#include "MCTargetDesc/EMBERBaseInfo.h"
#include "MCTargetDesc/EMBERFixupKinds.h"
#include "MCTargetDesc/EMBERMCExpr.h"
#include "llvm/ADT/Statistic.h"
// #include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCInstrInfo.h"
// #include "llvm/MC/MCRegisterInfo.h"
// #include "llvm/MC/MCSymbol.h"
// #include "llvm/Support/Casting.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/MCSubtargetInfo.h"



using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");
STATISTIC(MCNumFixups, "Number of MC fixups created");

namespace 
{

class EMBERMCCodeEmitter : public MCCodeEmitter 
{
    EMBERMCCodeEmitter(const EMBERMCCodeEmitter &) = delete;
    void operator=(const EMBERMCCodeEmitter &) = delete;
    MCContext &Ctx;
    MCInstrInfo const &MCII;

public:
    EMBERMCCodeEmitter(MCContext &ctx, MCInstrInfo const &MCII) :
        Ctx(ctx),
        MCII(MCII) 
    {}

    ~EMBERMCCodeEmitter() override {}

    void encodeInstruction(const MCInst             &MI,
                           raw_ostream              &OS,
                           SmallVectorImpl<MCFixup> &Fixups,
                           const MCSubtargetInfo    &STI) const override;

  void expandFunctionCall(const MCInst &MI, raw_ostream &OS,
                          SmallVectorImpl<MCFixup> &Fixups,
                          const MCSubtargetInfo &STI) const;

  void expandAddTPRel(const MCInst &MI, raw_ostream &OS,
                      SmallVectorImpl<MCFixup> &Fixups,
                      const MCSubtargetInfo &STI) const;

    /// TableGen'erated function for getting the binary encoding for an
    /// instruction.
    uint64_t getBinaryCodeForInstr(const MCInst             &MI,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo    &STI) const;

    /// Return binary encoding of operand. If the machine operand requires
    /// relocation, record the relocation and return zero.
    unsigned getMachineOpValue(const MCInst             &MI, 
                               const MCOperand          &MO,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo    &STI) const;

    unsigned getBranchTargetOpValueSImm22(const MCInst             &MI,
                                          unsigned                 operandNo,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo    &STI) const;

  unsigned getImmOpValue(const MCInst &MI, unsigned OpNo,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const;

  unsigned getVMaskReg(const MCInst &MI, unsigned OpNo,
                       SmallVectorImpl<MCFixup> &Fixups,
                       const MCSubtargetInfo &STI) const;

private:
    FeatureBitset computeAvailableFeatures(const FeatureBitset &FB) const;

    void verifyInstructionPredicates(const MCInst &MI, const FeatureBitset &AvailableFeatures) const;
};
} // end anonymous namespace

MCCodeEmitter *llvm::createEMBERMCCodeEmitter(const MCInstrInfo    &MCII,
                                              const MCRegisterInfo &MRI,
                                              MCContext            &Ctx) 
{
    return new EMBERMCCodeEmitter(Ctx, MCII);
}

/*

// Expand PseudoCALL(Reg), PseudoTAIL and PseudoJump to AUIPC and JALR with
// relocation types. We expand those pseudo-instructions while encoding them,
// meaning AUIPC and JALR won't go through EMBER MC to MC compressed
// instruction transformation. This is acceptable because AUIPC has no 16-bit
// form and C_JALR has no immediate operand field.  We let linker relaxation
// deal with it. When linker relaxation is enabled, AUIPC and JALR have a
// chance to relax to JAL.
// If the C extension is enabled, JAL has a chance relax to C_JAL.
void EMBERMCCodeEmitter::expandFunctionCall(const MCInst &MI, raw_ostream &OS,
                                            SmallVectorImpl<MCFixup> &Fixups,
                                            const MCSubtargetInfo &STI) const {
  MCInst TmpInst;
  MCOperand Func;
  MCRegister Ra;
  if (MI.getOpcode() == EMBER::PseudoTAIL) {
    Func = MI.getOperand(0);
    Ra = EMBER::X6;
  } else if (MI.getOpcode() == EMBER::PseudoCALLReg) {
    Func = MI.getOperand(1);
    Ra = MI.getOperand(0).getReg();
  } else if (MI.getOpcode() == EMBER::PseudoCALL) {
    Func = MI.getOperand(0);
    Ra = EMBER::X1;
  } else if (MI.getOpcode() == EMBER::PseudoJump) {
    Func = MI.getOperand(1);
    Ra = MI.getOperand(0).getReg();
  }
  uint32_t Binary;

  assert(Func.isExpr() && "Expected expression");

  const MCExpr *CallExpr = Func.getExpr();

  // Emit AUIPC Ra, Func with R_EMBER_CALL relocation type.
  TmpInst = MCInstBuilder(EMBER::AUIPC)
                .addReg(Ra)
                .addOperand(MCOperand::createExpr(CallExpr));
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  support::endian::write(OS, Binary, support::little);

  if (MI.getOpcode() == EMBER::PseudoTAIL ||
      MI.getOpcode() == EMBER::PseudoJump)
    // Emit JALR X0, Ra, 0
    TmpInst = MCInstBuilder(EMBER::JALR).addReg(EMBER::X0).addReg(Ra).addImm(0);
  else
    // Emit JALR Ra, Ra, 0
    TmpInst = MCInstBuilder(EMBER::JALR).addReg(Ra).addReg(Ra).addImm(0);
  Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  support::endian::write(OS, Binary, support::little);
}

// Expand PseudoAddTPRel to a simple ADD with the correct relocation.
void EMBERMCCodeEmitter::expandAddTPRel(const MCInst &MI, raw_ostream &OS,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  MCOperand DestReg = MI.getOperand(0);
  MCOperand SrcReg = MI.getOperand(1);
  MCOperand TPReg = MI.getOperand(2);
  assert(TPReg.isReg() && TPReg.getReg() == EMBER::X4 &&
         "Expected thread pointer as second input to TP-relative add");

  MCOperand SrcSymbol = MI.getOperand(3);
  assert(SrcSymbol.isExpr() &&
         "Expected expression as third input to TP-relative add");

  const EMBERMCExpr *Expr = dyn_cast<EMBERMCExpr>(SrcSymbol.getExpr());
  assert(Expr && Expr->getKind() == EMBERMCExpr::VK_EMBER_TPREL_ADD &&
         "Expected tprel_add relocation on TP-relative symbol");

  // Emit the correct tprel_add relocation for the symbol.
  Fixups.push_back(MCFixup::create(
      0, Expr, MCFixupKind(EMBER::fixup_riscv_tprel_add), MI.getLoc()));

  // Emit fixup_riscv_relax for tprel_add where the relax feature is enabled.
  if (STI.getFeatureBits()[EMBER::FeatureRelax]) {
    const MCConstantExpr *Dummy = MCConstantExpr::create(0, Ctx);
    Fixups.push_back(MCFixup::create(
        0, Dummy, MCFixupKind(EMBER::fixup_riscv_relax), MI.getLoc()));
  }

  // Emit a normal ADD instruction with the given operands.
  MCInst TmpInst = MCInstBuilder(EMBER::ADD)
                       .addOperand(DestReg)
                       .addOperand(SrcReg)
                       .addOperand(TPReg);
  uint32_t Binary = getBinaryCodeForInstr(TmpInst, Fixups, STI);
  support::endian::write(OS, Binary, support::little);
}
*/
void EMBERMCCodeEmitter::encodeInstruction(const MCInst             &MI, 
                                           raw_ostream              &OS,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo    &STI) const
{
    verifyInstructionPredicates(MI, computeAvailableFeatures(STI.getFeatureBits()));

    const MCInstrDesc &Desc = MCII.get(MI.getOpcode());

    // Get byte count of instruction.
    unsigned Size = Desc.getSize();

    // EMBERInstrInfo::getInstSizeInBytes hard-codes the number of expanded
    // instructions for each pseudo, and must be updated when adding new pseudos
    // or changing existing ones.
//   if (MI.getOpcode() == EMBER::PseudoCALLReg ||
//       MI.getOpcode() == EMBER::PseudoCALL ||
//       MI.getOpcode() == EMBER::PseudoTAIL ||
//       MI.getOpcode() == EMBER::PseudoJump) {
//     expandFunctionCall(MI, OS, Fixups, STI);
//     MCNumEmitted += 2;
//     return;
//   }

//   if (MI.getOpcode() == EMBER::PseudoAddTPRel) {
//     expandAddTPRel(MI, OS, Fixups, STI);
//     MCNumEmitted += 1;
//     return;
//   }

    switch (Size) 
    {
        default:
            llvm_unreachable("Unhandled encodeInstruction length!");
        case 8: 
        {
            uint64_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
            support::endian::write<uint64_t>(OS, Bits, support::little);
            break;
        }
        case 4:
        {
            uint32_t Bits = getBinaryCodeForInstr(MI, Fixups, STI);
            support::endian::write(OS, Bits, support::little);
            break;
        }
    }

    ++MCNumEmitted; // Keep track of the # of mi's emitted.
}


unsigned EMBERMCCodeEmitter::getMachineOpValue(const MCInst             &MI,
                                               const MCOperand          &MO,
                                               SmallVectorImpl<MCFixup> &Fixups,
                                               const MCSubtargetInfo    &STI) const
{
    if (MO.isReg())
      return Ctx.getRegisterInfo()->getEncodingValue(MO.getReg());

    if (MO.isImm())
      return static_cast<unsigned>(MO.getImm());

    return 0;
}

unsigned EMBERMCCodeEmitter::getBranchTargetOpValueSImm22(const MCInst             &MI,
                                                          unsigned                 operandNo,
                                                          SmallVectorImpl<MCFixup> &Fixups,
                                                          const MCSubtargetInfo    &STI) const
{
    const MCOperand& MO = MI.getOperand(operandNo);
    const MCExpr* Expr = MO.getExpr();
    MCExpr::ExprKind Kind = Expr->getKind();

    // Since it has to be a symbol, we can't resolve it until later, so add a 'fixup'
    EMBER::Fixups FixupKind = EMBER::fixup_ember_invalid;
    if (Kind == MCExpr::SymbolRef && cast<MCSymbolRefExpr>(Expr)->getKind() == MCSymbolRefExpr::VK_None)
    {
        FixupKind = EMBER::fixup_ember_branch;

        Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(FixupKind), MI.getLoc()));
        ++MCNumFixups;

        // Just return 0 now, fixup will get actual value later
        return 0;
    }

    assert(0 && "Unhandled immediate branch target expression!");
    return 0;
}

/*
unsigned
EMBERMCCodeEmitter::getImmOpValueAsr1(const MCInst &MI, unsigned OpNo,
                                      SmallVectorImpl<MCFixup> &Fixups,
                                      const MCSubtargetInfo &STI) const {
  const MCOperand &MO = MI.getOperand(OpNo);

  if (MO.isImm()) {
    unsigned Res = MO.getImm();
    assert((Res & 1) == 0 && "LSB is non-zero");
    return Res >> 1;
  }

  return getImmOpValue(MI, OpNo, Fixups, STI);
}
*/


unsigned EMBERMCCodeEmitter::getImmOpValue(const MCInst             &MI, 
                                           unsigned                  OpNo,
                                           SmallVectorImpl<MCFixup> &Fixups,
                                           const MCSubtargetInfo    &STI) const 
{
//   bool EnableRelax = STI.getFeatureBits()[EMBER::FeatureRelax];
    const MCOperand &MO = MI.getOperand(OpNo);
// 
//    unsigned MIFrm = Desc.TSFlags & EMBERII::InstFormatMask;
// 
    // If the destination is an immediate, there is nothing to do.
    if (MO.isImm())
      return MO.getImm();

    assert(MO.isExpr() && "getImmOpValue expects only expressions or immediates");

    MCInstrDesc const &Desc = MCII.get(MI.getOpcode());
    const MCExpr *Expr = MO.getExpr();
    MCExpr::ExprKind Kind = Expr->getKind();
    EMBER::Fixups FixupKind = EMBER::fixup_ember_invalid;
//   bool RelaxCandidate = false;
    if (Kind == MCExpr::Target) 
    {
//        const EMBERMCExpr *RVExpr = cast<EMBERMCExpr>(Expr);

//        switch (RVExpr->getKind())
        {
//            case EMBERMCExpr::VK_EMBER_None:
//            case EMBERMCExpr::VK_EMBER_Invalid:

            
            
//     case EMBERMCExpr::VK_EMBER_32_PCREL:
//       llvm_unreachable("Unhandled fixup kind!");
//     case EMBERMCExpr::VK_EMBER_TPREL_ADD:
//       // tprel_add is only used to indicate that a relocation should be emitted
//       // for an add instruction used in TP-relative addressing. It should not be
//       // expanded as if representing an actual instruction operand and so to
//       // encounter it here is an error.
//       llvm_unreachable(
//           "VK_EMBER_TPREL_ADD should not represent an instruction operand");
//     case EMBERMCExpr::VK_EMBER_LO:
//       if (MIFrm == EMBERII::InstFormatI)
//         FixupKind = EMBER::fixup_riscv_lo12_i;
//       else if (MIFrm == EMBERII::InstFormatS)
//         FixupKind = EMBER::fixup_riscv_lo12_s;
//       else
//         llvm_unreachable("VK_EMBER_LO used with unexpected instruction format");
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_HI:
//       FixupKind = EMBER::fixup_riscv_hi20;
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_PCREL_LO:
//       if (MIFrm == EMBERII::InstFormatI)
//         FixupKind = EMBER::fixup_riscv_pcrel_lo12_i;
//       else if (MIFrm == EMBERII::InstFormatS)
//         FixupKind = EMBER::fixup_riscv_pcrel_lo12_s;
//       else
//         llvm_unreachable(
//             "VK_EMBER_PCREL_LO used with unexpected instruction format");
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_PCREL_HI:
//       FixupKind = EMBER::fixup_riscv_pcrel_hi20;
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_GOT_HI:
//       FixupKind = EMBER::fixup_riscv_got_hi20;
//       break;
//     case EMBERMCExpr::VK_EMBER_TPREL_LO:
//       if (MIFrm == EMBERII::InstFormatI)
//         FixupKind = EMBER::fixup_riscv_tprel_lo12_i;
//       else if (MIFrm == EMBERII::InstFormatS)
//         FixupKind = EMBER::fixup_riscv_tprel_lo12_s;
//       else
//         llvm_unreachable(
//             "VK_EMBER_TPREL_LO used with unexpected instruction format");
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_TPREL_HI:
//       FixupKind = EMBER::fixup_riscv_tprel_hi20;
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_TLS_GOT_HI:
//       FixupKind = EMBER::fixup_riscv_tls_got_hi20;
//       break;
//     case EMBERMCExpr::VK_EMBER_TLS_GD_HI:
//       FixupKind = EMBER::fixup_riscv_tls_gd_hi20;
//       break;
//     case EMBERMCExpr::VK_EMBER_CALL:
//       FixupKind = EMBER::fixup_riscv_call;
//       RelaxCandidate = true;
//       break;
//     case EMBERMCExpr::VK_EMBER_CALL_PLT:
//       FixupKind = EMBER::fixup_riscv_call_plt;
//       RelaxCandidate = true;
//       break;
        }
    }
    else if (Kind == MCExpr::SymbolRef && cast<MCSymbolRefExpr>(Expr)->getKind() == MCSymbolRefExpr::VK_None) 
    {
        switch (Desc.getOpcode())
        {
            case EMBER::LDI_al_lo:
            case EMBER::LDI_c_lo:
            case EMBER::LDI_eq_lo:
            case EMBER::LDI_ge_lo:
            case EMBER::LDI_nc_lo:
            case EMBER::LDI_ne_lo:
            case EMBER::LDI_ng_lo:
            case EMBER::LDI_v_lo:
                FixupKind = EMBER::fixup_ember_ldi_label_addr_lo;
                break;
            case EMBER::LDI_al_hi:
            case EMBER::LDI_c_hi:
            case EMBER::LDI_eq_hi:
            case EMBER::LDI_ge_hi:
            case EMBER::LDI_nc_hi:
            case EMBER::LDI_ne_hi:
            case EMBER::LDI_ng_hi:
            case EMBER::LDI_v_hi:
                FixupKind = EMBER::fixup_ember_ldi_label_addr_hi;
                break;
            default:
                FixupKind = EMBER::fixup_ember_label_addr;
                break;
        }
    }

    assert(FixupKind != EMBER::fixup_ember_invalid && "Unhandled immediate value expression!");

    Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(FixupKind), MI.getLoc()));
    ++MCNumFixups;

//   // Ensure an R_EMBER_RELAX relocation will be emitted if linker relaxation is
//   // enabled and the current fixup will result in a relocation that may be
//   // relaxed.
//   if (EnableRelax && RelaxCandidate) {
//     const MCConstantExpr *Dummy = MCConstantExpr::create(0, Ctx);
//     Fixups.push_back(
//     MCFixup::create(0, Dummy, MCFixupKind(EMBER::fixup_riscv_relax),
//                     MI.getLoc()));
//     ++MCNumFixups;
//   }

    return 0;
}

/*
unsigned EMBERMCCodeEmitter::getVMaskReg(const MCInst &MI, unsigned OpNo,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  MCOperand MO = MI.getOperand(OpNo);
  assert(MO.isReg() && "Expected a register.");

  switch (MO.getReg()) {
  default:
    llvm_unreachable("Invalid mask register.");
  case EMBER::V0:
    return 0;
  case EMBER::NoRegister:
    return 1;
  }
}
*/

#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "EMBERGenMCCodeEmitter.inc"

