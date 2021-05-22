//===-- EMBERELFObjectWriter.cpp - EMBER ELF Writer -----------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/EMBERFixupKinds.h"
#include "MCTargetDesc/EMBERMCExpr.h"
#include "MCTargetDesc/EMBERMCTargetDesc.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;


namespace {
class EMBERELFObjectWriter : public MCELFObjectTargetWriter
{
public:
  EMBERELFObjectWriter();
  ~EMBERELFObjectWriter() override;

  // Return true if the given relocation must be with a symbol rather than
  // section plus offset.
  bool needsRelocateWithSymbol(const MCSymbol &Sym,
                               unsigned Type) const override {
    // TODO: this is very conservative, update once RISC-V psABI requirements
    //       are clarified.
    return true;
  }

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel) const override;
};
}

EMBERELFObjectWriter::EMBERELFObjectWriter()
    : MCELFObjectTargetWriter(false, ELF::ELFOSABI_NONE /*TODO Do we need to add one of these, or use one?*/, ELF::EM_EMBER, /*HasRelocationAddend*/ true) {}

EMBERELFObjectWriter::~EMBERELFObjectWriter() {}

unsigned EMBERELFObjectWriter::getRelocType(MCContext &Ctx,
                                            const MCValue &Target,
                                            const MCFixup &Fixup,
                                            bool IsPCRel) const {
//   const MCExpr *Expr = Fixup.getValue();
//   // Determine the type of the relocation
//   unsigned Kind = Fixup.getTargetKind();

  // TODO:
  return ELF::R_EMBER_NONE;
  /*
  if (Kind >= FirstLiteralRelocationKind)
    return Kind - FirstLiteralRelocationKind;
  if (IsPCRel) {
    switch (Kind) {
    default:
      Ctx.reportError(Fixup.getLoc(), "Unsupported relocation type");
      return ELF::R_EMBER_NONE;
    case FK_Data_4:
    case FK_PCRel_4:
      return ELF::R_EMBER_32_PCREL;
    case EMBER::fixup_riscv_pcrel_hi20:
      return ELF::R_EMBER_PCREL_HI20;
    case EMBER::fixup_riscv_pcrel_lo12_i:
      return ELF::R_EMBER_PCREL_LO12_I;
    case EMBER::fixup_riscv_pcrel_lo12_s:
      return ELF::R_EMBER_PCREL_LO12_S;
    case EMBER::fixup_riscv_got_hi20:
      return ELF::R_EMBER_GOT_HI20;
    case EMBER::fixup_riscv_tls_got_hi20:
      return ELF::R_EMBER_TLS_GOT_HI20;
    case EMBER::fixup_riscv_tls_gd_hi20:
      return ELF::R_EMBER_TLS_GD_HI20;
    case EMBER::fixup_riscv_jal:
      return ELF::R_EMBER_JAL;
    case EMBER::fixup_riscv_branch:
      return ELF::R_EMBER_BRANCH;
    case EMBER::fixup_riscv_rvc_jump:
      return ELF::R_EMBER_RVC_JUMP;
    case EMBER::fixup_riscv_rvc_branch:
      return ELF::R_EMBER_RVC_BRANCH;
    case EMBER::fixup_riscv_call:
      return ELF::R_EMBER_CALL;
    case EMBER::fixup_riscv_call_plt:
      return ELF::R_EMBER_CALL_PLT;
    }
  }

  switch (Kind) {
  default:
    Ctx.reportError(Fixup.getLoc(), "Unsupported relocation type");
    return ELF::R_EMBER_NONE;
  case FK_Data_1:
    Ctx.reportError(Fixup.getLoc(), "1-byte data relocations not supported");
    return ELF::R_EMBER_NONE;
  case FK_Data_2:
    Ctx.reportError(Fixup.getLoc(), "2-byte data relocations not supported");
    return ELF::R_EMBER_NONE;
  case FK_Data_4:
    if (Expr->getKind() == MCExpr::Target &&
        cast<EMBERMCExpr>(Expr)->getKind() == EMBERMCExpr::VK_EMBER_32_PCREL)
      return ELF::R_EMBER_32_PCREL;
    return ELF::R_EMBER_32;
  case FK_Data_8:
    return ELF::R_EMBER_64;
  case FK_Data_Add_1:
    return ELF::R_EMBER_ADD8;
  case FK_Data_Add_2:
    return ELF::R_EMBER_ADD16;
  case FK_Data_Add_4:
    return ELF::R_EMBER_ADD32;
  case FK_Data_Add_8:
    return ELF::R_EMBER_ADD64;
  case FK_Data_Add_6b:
    return ELF::R_EMBER_SET6;
  case FK_Data_Sub_1:
    return ELF::R_EMBER_SUB8;
  case FK_Data_Sub_2:
    return ELF::R_EMBER_SUB16;
  case FK_Data_Sub_4:
    return ELF::R_EMBER_SUB32;
  case FK_Data_Sub_8:
    return ELF::R_EMBER_SUB64;
  case FK_Data_Sub_6b:
    return ELF::R_EMBER_SUB6;
  case EMBER::fixup_riscv_hi20:
    return ELF::R_EMBER_HI20;
  case EMBER::fixup_riscv_lo12_i:
    return ELF::R_EMBER_LO12_I;
  case EMBER::fixup_riscv_lo12_s:
    return ELF::R_EMBER_LO12_S;
  case EMBER::fixup_riscv_tprel_hi20:
    return ELF::R_EMBER_TPREL_HI20;
  case EMBER::fixup_riscv_tprel_lo12_i:
    return ELF::R_EMBER_TPREL_LO12_I;
  case EMBER::fixup_riscv_tprel_lo12_s:
    return ELF::R_EMBER_TPREL_LO12_S;
  case EMBER::fixup_riscv_tprel_add:
    return ELF::R_EMBER_TPREL_ADD;
  case EMBER::fixup_riscv_relax:
    return ELF::R_EMBER_RELAX;
  case EMBER::fixup_riscv_align:
    return ELF::R_EMBER_ALIGN;
  }
*/
}
std::unique_ptr<MCObjectTargetWriter> llvm::createEMBERELFObjectWriter()
{
  return std::make_unique<EMBERELFObjectWriter>();
}

