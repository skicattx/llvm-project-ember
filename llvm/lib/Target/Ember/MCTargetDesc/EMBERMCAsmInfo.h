//===-- EMBERMCAsmInfo.h - EMBER Asm Info ----------------------*- C++ -*--===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declaration of the EMBERMCAsmInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERMCASMINFO_H
#define LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERMCASMINFO_H

#include "llvm/MC/MCAsmInfoELF.h"

namespace llvm {
class Triple;

class EMBERMCAsmInfo : public MCAsmInfoELF {
  void anchor() override;

public:
  explicit EMBERMCAsmInfo(const Triple &TargetTriple);

//   const MCExpr *getExprForFDESymbol(const MCSymbol *Sym, unsigned Encoding,
//                                     MCStreamer &Streamer) const override;
};

} // namespace llvm

#endif
