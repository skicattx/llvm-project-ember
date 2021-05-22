//===-- EMBERMCExpr.h - EMBER specific MC expression classes ----*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file describes EMBER-specific MCExprs, used for modifiers like
// "%hi" or "%lo" etc.,
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERMCEXPR_H
#define LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERMCEXPR_H

#include "llvm/MC/MCExpr.h"

namespace llvm {


class StringRef;

class EMBERMCExpr : public MCTargetExpr {
public:
  enum VariantKind {
    VK_EMBER_None,
    VK_EMBER_LO,
    VK_EMBER_HI,
    VK_EMBER_PCREL_LO,
    VK_EMBER_PCREL_HI,
    VK_EMBER_GOT_HI,
    VK_EMBER_TPREL_LO,
    VK_EMBER_TPREL_HI,
    VK_EMBER_TPREL_ADD,
    VK_EMBER_TLS_GOT_HI,
    VK_EMBER_TLS_GD_HI,
    VK_EMBER_CALL,
    VK_EMBER_CALL_PLT,
    VK_EMBER_32_PCREL,
    VK_EMBER_Invalid // Must be the last item
  };

private:
  const MCExpr *Expr;
  const VariantKind Kind;

/*

  int64_t evaluateAsInt64(int64_t Value) const;

  explicit EMBERMCExpr(const MCExpr *Expr, VariantKind Kind)
      : Expr(Expr), Kind(Kind) {}
*/
public:
/*
  static const EMBERMCExpr *create(const MCExpr *Expr, VariantKind Kind,
                                   MCContext &Ctx);

  VariantKind getKind() const { return Kind; }
  */
  const MCExpr *getSubExpr() const { return Expr; }
  /*
  /// Get the corresponding PC-relative HI fixup that a VK_EMBER_PCREL_LO
  /// points to, and optionally the fragment containing it.
  ///
  /// \returns nullptr if this isn't a VK_EMBER_PCREL_LO pointing to a
  /// known PC-relative HI fixup.
  const MCFixup *getPCRelHiFixup(const MCFragment **DFOut) const;

  void printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const override;
  bool evaluateAsRelocatableImpl(MCValue &Res, const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;
  void visitUsedExpr(MCStreamer &Streamer) const override;
  MCFragment *findAssociatedFragment() const override {
    return getSubExpr()->findAssociatedFragment();
  }

  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const override;
  */
  bool evaluateAsConstant(int64_t &Res) const;
  /*
  static bool classof(const MCExpr *E) {
    return E->getKind() == MCExpr::Target;
  }

  static bool classof(const EMBERMCExpr *) { return true; }

  static VariantKind getVariantKindForName(StringRef name);
  static StringRef getVariantKindName(VariantKind Kind);

*/

};


} // end namespace llvm.

#endif
