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
//    const MCTargetOptions &TargetOptions;

public:
    EMBERAsmBackend(const MCSubtargetInfo &STI, uint8_t OSABI) :
        MCAsmBackend(support::little),
        STI(STI) 
    {
    }
    ~EMBERAsmBackend() override 
    {
    }

    std::unique_ptr<MCObjectTargetWriter> createObjectTargetWriter() const override;

    unsigned getNumFixupKinds() const override 
    {
        return EMBER::NumTargetFixupKinds;
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

 
    Optional<MCFixupKind> getFixupKind(StringRef Name) const override;

    const MCFixupKindInfo &getFixupKindInfo(MCFixupKind Kind) const override;

};

}

#endif
