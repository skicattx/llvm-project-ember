//===-- EMBERAsmBackend.h - EMBER Assembler Backend -----------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERASMBACKEND_H
#define LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERASMBACKEND_H

#include "MCTargetDesc/EMBERBaseInfo.h"
#include "MCTargetDesc/EMBERFixupKinds.h"
#include "MCTargetDesc/EMBERMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"

namespace llvm {
class MCAssembler;
class MCObjectTargetWriter;
class raw_ostream;
class EMBERAsmBackend : public MCAsmBackend 
{
    const MCSubtargetInfo &STI;
    const MCTargetOptions &TargetOptions;
/*
  uint8_t OSABI;
  bool ForceRelocs = false;
  EMBERABI::ABI TargetABI = EMBERABI::ABI_Unknown;
*/
public:
    EMBERAsmBackend(const MCSubtargetInfo &STI, uint8_t OSABI, const MCTargetOptions &Options) :
        MCAsmBackend(support::little),
        STI(STI), 
//       OSABI(OSABI),
        TargetOptions(Options) 
    {
//      TargetABI = EMBERABI::computeTargetABI(STI.getTargetTriple(), STI.getFeatureBits(), Options.getABIName());
//      EMBERFeatures::validate(STI.getTargetTriple(), STI.getFeatureBits());
    }
    ~EMBERAsmBackend() override {}

  std::unique_ptr<MCObjectTargetWriter> createObjectTargetWriter() const override;

  unsigned getNumFixupKinds() const override 
  {
    return 1;/*EMBER::NumTargetFixupKinds*/;
  }

  void applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                  const MCValue &Target, MutableArrayRef<char> Data,
                  uint64_t Value, bool IsResolved,
                  const MCSubtargetInfo *STI) const override;

  bool fixupNeedsRelaxation(const MCFixup &Fixup, uint64_t Value,
                            const MCRelaxableFragment *DF,
                            const MCAsmLayout &Layout) const override 
  {
    llvm_unreachable("Handled by fixupNeedsRelaxationAdvanced");
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count) const override;

/*
    void setForceRelocs() { ForceRelocs = true; }

  // Returns true if relocations will be forced for shouldForceRelocation by
  // default. This will be true if relaxation is enabled or had previously
  // been enabled.
  bool willForceRelocations() const {
    return ForceRelocs || STI.getFeatureBits()[EMBER::FeatureRelax];
  }
  // Generate diff expression relocations if the relax feature is enabled or had
  // previously been enabled, otherwise it is safe for the assembler to
  // calculate these internally.
  bool requiresDiffExpressionRelocations() const override {
    return willForceRelocations();
  }

  // Return Size with extra Nop Bytes for alignment directive in code section.
  bool shouldInsertExtraNopBytesForCodeAlign(const MCAlignFragment &AF,
                                             unsigned &Size) override;

  // Insert target specific fixup type for alignment directive in code section.
  bool shouldInsertFixupForCodeAlign(MCAssembler &Asm,
                                     const MCAsmLayout &Layout,
                                     MCAlignFragment &AF) override;

  bool evaluateTargetFixup(const MCAssembler &Asm, const MCAsmLayout &Layout,
                           const MCFixup &Fixup, const MCFragment *DF,
                           const MCValue &Target, uint64_t &Value,
                           bool &WasForced) override;

  bool shouldForceRelocation(const MCAssembler &Asm, const MCFixup &Fixup,
                             const MCValue &Target) override;

  bool fixupNeedsRelaxationAdvanced(const MCFixup &Fixup, bool Resolved,
                                    uint64_t Value,
                                    const MCRelaxableFragment *DF,
                                    const MCAsmLayout &Layout,
                                    const bool WasForced) const override;
*/
  
    Optional<MCFixupKind> getFixupKind(StringRef Name) const override;

    const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

/*
  bool mayNeedRelaxation(const MCInst &Inst,
                         const MCSubtargetInfo &STI) const override;
  unsigned getRelaxedOpcode(unsigned Op) const;

  void relaxInstruction(MCInst &Inst,
                        const MCSubtargetInfo &STI) const override;

  const MCTargetOptions &getTargetOptions() const { return TargetOptions; }
  EMBERABI::ABI getTargetABI() const { return TargetABI; }
*/
};

}

#endif
