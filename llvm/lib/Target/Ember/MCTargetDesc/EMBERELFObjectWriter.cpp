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
    return true;
  }

protected:
  unsigned getRelocType(MCContext &Ctx, const MCValue &Target, const MCFixup &Fixup, bool IsPCRel) const override;
};
}

EMBERELFObjectWriter::EMBERELFObjectWriter()
    : MCELFObjectTargetWriter(false, ELF::ELFOSABI_NONE /*TODO Do we need to add one of these, or use one?*/, ELF::EM_EMBER, /*HasRelocationAddend*/ false) {}

EMBERELFObjectWriter::~EMBERELFObjectWriter() {}

unsigned EMBERELFObjectWriter::getRelocType(MCContext      &Ctx,
                                            const MCValue  &Target,
                                            const MCFixup  &Fixup,
                                            bool            IsPCRel) const 
{
//     const MCExpr *Expr = Fixup.getValue();

    // Determine the type of the relocation
    unsigned Kind = Fixup.getTargetKind();

//    Ctx.reportError(Fixup.getLoc(), "Label is Undefined");

    if (Kind >= FirstLiteralRelocationKind)
        return Kind - FirstLiteralRelocationKind;
    switch (Kind) 
    {
        default:
            Ctx.reportError(Fixup.getLoc(), "Unsupported relocation type");
            return ELF::R_EMBER_NONE;
        case FK_Data_4:
            return (IsPCRel) ? ELF::R_EMBER_32_PCREL : ELF::R_EMBER_32; // For now this is "unknown" 32-bit relocation (will be saved in ELF) probably debug/dwarf related?
        case EMBER::fixup_ember_branch:
            return ELF::R_EMBER_BRANCH;
        case EMBER::fixup_ember_ldi_label_addr_lo:
            return ELF::R_EMBER_LDI_LABEL_ADDR_LO;
        case EMBER::fixup_ember_ldi_label_addr_hi:
            return ELF::R_EMBER_LDI_LABEL_ADDR_HI;
    }

}

std::unique_ptr<MCObjectTargetWriter> llvm::createEMBERELFObjectWriter()
{
    return std::make_unique<EMBERELFObjectWriter>();
}

