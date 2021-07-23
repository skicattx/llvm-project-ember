//===-- EMBERAsmBackend.cpp - EMBER Assembler Backend ---------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#include "EMBERAsmBackend.h"
#include "EMBERMCExpr.h"
#include "llvm/ADT/APInt.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

Optional<MCFixupKind> EMBERAsmBackend::getFixupKind(StringRef Name) const 
{
    if (STI.getTargetTriple().isOSBinFormatELF()) 
    {
        unsigned Type;
        Type = llvm::StringSwitch<unsigned>(Name)
#define ELF_RELOC(X, Y) .Case(#X, Y)
#include "llvm/BinaryFormat/ELFRelocs/EMBER.def"
#undef ELF_RELOC
               .Case("BFD_RELOC_NONE", ELF::R_EMBER_NONE)
               .Case("BFD_RELOC_32", ELF::R_EMBER_32)
               .Default(-1u);
        if (Type != -1u)
            return static_cast<MCFixupKind>(FirstLiteralRelocationKind + Type);
    }
    return None;
}

const MCFixupKindInfo &EMBERAsmBackend::getFixupKindInfo(MCFixupKind Kind) const 
{
    const static MCFixupKindInfo Infos[] = 
    {
      // This table *must* be in the order that the fixup_* kinds are defined in
      // EMBERFixupKinds.h.
      //
      // name                           offset bits    flags
      {"fixup_ember_branch",            0,     22,     MCFixupKindInfo::FKF_IsTarget | MCFixupKindInfo::FKF_IsAlignedDownTo32Bits | MCFixupKindInfo::FKF_IsPCRel},
      {"fixup_ember_label_addr",        0,     14,     MCFixupKindInfo::FKF_IsTarget | MCFixupKindInfo::FKF_IsAlignedDownTo32Bits},  
      {"fixup_ember_ldi_label_addr_lo", 0,     16,     MCFixupKindInfo::FKF_IsTarget | MCFixupKindInfo::FKF_IsAlignedDownTo32Bits},
      {"fixup_ember_ldi_label_addr_hi", 0,     16,     MCFixupKindInfo::FKF_IsTarget | MCFixupKindInfo::FKF_IsAlignedDownTo32Bits} };  // TODO: need one of these for each label imm bit count

    static_assert((array_lengthof(Infos)) == EMBER::NumTargetFixupKinds, "Not all fixup kinds added to Infos array");

    // Not handled here, pass to default implementation if they are out of range
    if (Kind >= FirstLiteralRelocationKind)
        return MCAsmBackend::getFixupKindInfo(FK_NONE);

    if (Kind < FirstTargetFixupKind)
        return MCAsmBackend::getFixupKindInfo(Kind);

    assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() && "Invalid Ember Fixup Kind!");
    return Infos[Kind - FirstTargetFixupKind];
}

bool EMBERAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count) const 
{

  return true;
}
/*
static uint64_t adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext &Ctx) {
  switch (Fixup.getTargetKind()) {
  default:
    llvm_unreachable("Unknown fixup kind!");
  case EMBER::fixup_riscv_got_hi20:
  case EMBER::fixup_riscv_tls_got_hi20:
  case EMBER::fixup_riscv_tls_gd_hi20:
    llvm_unreachable("Relocation should be unconditionally forced\n");
  case FK_Data_1:
  case FK_Data_2:
  case FK_Data_4:
  case FK_Data_8:
  case FK_Data_6b:
    return Value;
  case EMBER::fixup_riscv_lo12_i:
  case EMBER::fixup_riscv_pcrel_lo12_i:
  case EMBER::fixup_riscv_tprel_lo12_i:
    return Value & 0xfff;
  case EMBER::fixup_riscv_lo12_s:
  case EMBER::fixup_riscv_pcrel_lo12_s:
  case EMBER::fixup_riscv_tprel_lo12_s:
    return (((Value >> 5) & 0x7f) << 25) | ((Value & 0x1f) << 7);
  case EMBER::fixup_riscv_hi20:
  case EMBER::fixup_riscv_pcrel_hi20:
  case EMBER::fixup_riscv_tprel_hi20:
    // Add 1 if bit 11 is 1, to compensate for low 12 bits being negative.
    return ((Value + 0x800) >> 12) & 0xfffff;
  case EMBER::fixup_riscv_jal: {
    if (!isInt<21>(Value))
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned");
    // Need to produce imm[19|10:1|11|19:12] from the 21-bit Value.
    unsigned Sbit = (Value >> 20) & 0x1;
    unsigned Hi8 = (Value >> 12) & 0xff;
    unsigned Mid1 = (Value >> 11) & 0x1;
    unsigned Lo10 = (Value >> 1) & 0x3ff;
    // Inst{31} = Sbit;
    // Inst{30-21} = Lo10;
    // Inst{20} = Mid1;
    // Inst{19-12} = Hi8;
    Value = (Sbit << 19) | (Lo10 << 9) | (Mid1 << 8) | Hi8;
    return Value;
  }
  case EMBER::fixup_riscv_branch: {
    if (!isInt<13>(Value))
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range");
    if (Value & 0x1)
      Ctx.reportError(Fixup.getLoc(), "fixup value must be 2-byte aligned");
    // Need to extract imm[12], imm[10:5], imm[4:1], imm[11] from the 13-bit
    // Value.
    unsigned Sbit = (Value >> 12) & 0x1;
    unsigned Hi1 = (Value >> 11) & 0x1;
    unsigned Mid6 = (Value >> 5) & 0x3f;
    unsigned Lo4 = (Value >> 1) & 0xf;
    // Inst{31} = Sbit;
    // Inst{30-25} = Mid6;
    // Inst{11-8} = Lo4;
    // Inst{7} = Hi1;
    Value = (Sbit << 31) | (Mid6 << 25) | (Lo4 << 8) | (Hi1 << 7);
    return Value;
  }
  case EMBER::fixup_riscv_call:
  case EMBER::fixup_riscv_call_plt: {
    // Jalr will add UpperImm with the sign-extended 12-bit LowerImm,
    // we need to add 0x800ULL before extract upper bits to reflect the
    // effect of the sign extension.
    uint64_t UpperImm = (Value + 0x800ULL) & 0xfffff000ULL;
    uint64_t LowerImm = Value & 0xfffULL;
    return UpperImm | ((LowerImm << 20) << 32);
  }
  case EMBER::fixup_riscv_rvc_jump: {
    // Need to produce offset[11|4|9:8|10|6|7|3:1|5] from the 11-bit Value.
    unsigned Bit11  = (Value >> 11) & 0x1;
    unsigned Bit4   = (Value >> 4) & 0x1;
    unsigned Bit9_8 = (Value >> 8) & 0x3;
    unsigned Bit10  = (Value >> 10) & 0x1;
    unsigned Bit6   = (Value >> 6) & 0x1;
    unsigned Bit7   = (Value >> 7) & 0x1;
    unsigned Bit3_1 = (Value >> 1) & 0x7;
    unsigned Bit5   = (Value >> 5) & 0x1;
    Value = (Bit11 << 10) | (Bit4 << 9) | (Bit9_8 << 7) | (Bit10 << 6) |
            (Bit6 << 5) | (Bit7 << 4) | (Bit3_1 << 1) | Bit5;
    return Value;
  }
  case EMBER::fixup_riscv_rvc_branch: {
    // Need to produce offset[8|4:3], [reg 3 bit], offset[7:6|2:1|5]
    unsigned Bit8   = (Value >> 8) & 0x1;
    unsigned Bit7_6 = (Value >> 6) & 0x3;
    unsigned Bit5   = (Value >> 5) & 0x1;
    unsigned Bit4_3 = (Value >> 3) & 0x3;
    unsigned Bit2_1 = (Value >> 1) & 0x3;
    Value = (Bit8 << 12) | (Bit4_3 << 10) | (Bit7_6 << 5) | (Bit2_1 << 3) |
            (Bit5 << 2);
    return Value;
  }

  }
}
*/
bool EMBERAsmBackend::evaluateTargetFixup(
    const MCAssembler   &Asm, 
    const MCAsmLayout   &Layout, 
    const MCFixup       &Fixup,
    const MCFragment    *DF, 
    const MCValue       &Target, 
    uint64_t            &Value,
    bool                &WasForced) 
{
    if (!Target.getSymA() || Target.getSymB())
      return false;

    const MCSymbolRefExpr *A = Target.getSymA();
    const MCSymbol &SA = A->getSymbol();
    if (A->getKind() != MCSymbolRefExpr::VK_None || SA.isUndefined())
        return false;

    auto *Writer = Asm.getWriterPtr();
    if (!Writer)
        return false;

    bool IsResolved = Writer->isSymbolRefDifferenceFullyResolvedImpl(Asm, SA, *DF, false, true);
    if (!IsResolved)
        return false;

    Value = Layout.getSymbolOffset(SA) + Target.getConstant();

    switch (Fixup.getTargetKind())
    {
        default:
            llvm_unreachable("Unexpected fixup kind!");
        case EMBER::fixup_ember_branch:
            Value -= Layout.getFragmentOffset(DF) + Fixup.getOffset();
            Value = ((int64_t)Value)>>2;
            break;
        case EMBER::fixup_ember_label_addr:
            break;
        case EMBER::fixup_ember_ldi_label_addr_lo:
            break;
        case EMBER::fixup_ember_ldi_label_addr_hi:
            Value = ((int64_t)Value)>>16;
            break;
    }

    if (shouldForceRelocation(Asm, Fixup, Target))
    {
        WasForced = true;
        return false;
    }

    return true;
}

void EMBERAsmBackend::applyFixup(
    const MCAssembler       &Asm, 
    const MCFixup           &Fixup,
    const MCValue           &Target,
    MutableArrayRef<char>    Data, 
    uint64_t                 Value,
    bool                     IsResolved,
    const MCSubtargetInfo   *STI) const 
{
    MCFixupKind Kind = Fixup.getKind();
    if (Kind >= FirstLiteralRelocationKind)
        return;

    MCContext &Ctx = Asm.getContext();
    MCFixupKindInfo Info = getFixupKindInfo(Kind);
    if (!Value)
        return; // Doesn't change encoding.

    // Shift the value into position.
    Value <<= Info.TargetOffset;

    unsigned Offset = Fixup.getOffset();
    unsigned NumBytes = alignTo(Info.TargetSize + Info.TargetOffset, 8) / 8;

    assert(Offset + NumBytes <= Data.size() && "Invalid fixup offset!");

    // For each byte of the fragment that the fixup touches, mask in the
    // bits from the fixup value.
    for (unsigned i = 0; i != NumBytes; ++i) 
    {
        Data[Offset + i] |= uint8_t((Value >> (i * 8)) & 0xff);
    }
}



/*
// Linker relaxation may change code size. We have to insert Nops
// for .align directive when linker relaxation enabled. So then Linker
// could satisfy alignment by removing Nops.
// The function return the total Nops Size we need to insert.
bool EMBERAsmBackend::shouldInsertExtraNopBytesForCodeAlign(
    const MCAlignFragment &AF, unsigned &Size) {
  // Calculate Nops Size only when linker relaxation enabled.
  if (!STI.getFeatureBits()[EMBER::FeatureRelax])
    return false;

  bool HasStdExtC = STI.getFeatureBits()[EMBER::FeatureStdExtC];
  unsigned MinNopLen = HasStdExtC ? 2 : 4;

  if (AF.getAlignment() <= MinNopLen) {
    return false;
  } else {
    Size = AF.getAlignment() - MinNopLen;
    return true;
  }
}

// We need to insert R_EMBER_ALIGN relocation type to indicate the
// position of Nops and the total bytes of the Nops have been inserted
// when linker relaxation enabled.
// The function insert fixup_riscv_align fixup which eventually will
// transfer to R_EMBER_ALIGN relocation type.
bool EMBERAsmBackend::shouldInsertFixupForCodeAlign(MCAssembler &Asm,
                                                    const MCAsmLayout &Layout,
                                                    MCAlignFragment &AF) {
  // Insert the fixup only when linker relaxation enabled.
  if (!STI.getFeatureBits()[EMBER::FeatureRelax])
    return false;

  // Calculate total Nops we need to insert. If there are none to insert
  // then simply return.
  unsigned Count;
  if (!shouldInsertExtraNopBytesForCodeAlign(AF, Count) || (Count == 0))
    return false;

  MCContext &Ctx = Asm.getContext();
  const MCExpr *Dummy = MCConstantExpr::create(0, Ctx);
  // Create fixup_riscv_align fixup.
  MCFixup Fixup =
      MCFixup::create(0, Dummy, MCFixupKind(EMBER::fixup_riscv_align), SMLoc());

  uint64_t FixedValue = 0;
  MCValue NopBytes = MCValue::get(Count);

  Asm.getWriter().recordRelocation(Asm, Layout, &AF, Fixup, NopBytes,
                                   FixedValue);

  return true;
}
*/

std::unique_ptr<MCObjectTargetWriter> EMBERAsmBackend::createObjectTargetWriter() const 
{
  return createEMBERELFObjectWriter();
}

MCAsmBackend *llvm::createEMBERAsmBackend(const Target &T,
                                          const MCSubtargetInfo &STI,
                                          const MCRegisterInfo &MRI,
                                          const MCTargetOptions &Options) 
{
    const Triple &TT = STI.getTargetTriple();
    uint8_t OSABI = MCELFObjectTargetWriter::getOSABI(TT.getOS());
    
    return new EMBERAsmBackend(STI, OSABI);
}
