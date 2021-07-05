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

class EMBERMCExpr : public MCTargetExpr
{
public:
  enum VariantKind 
  {
    VK_EMBER_None,
    VK_EMBER_LO,
    VK_EMBER_HI,
    VK_EMBER_PCREL_LO,
    VK_EMBER_PCREL_HI,
    VK_EMBER_Invalid // Must be the last item
  };

private:
  const MCExpr *Expr;
  const VariantKind Kind;

  explicit EMBERMCExpr(const MCExpr *Expr, VariantKind Kind)
      : Expr(Expr), Kind(Kind)
  {}

public:
  static const EMBERMCExpr *create(const MCExpr *Expr, VariantKind Kind,
                                   MCContext &Ctx);

  VariantKind getKind() const { return Kind; }
  const MCExpr *getSubExpr() const { return Expr; }

  bool evaluateAsConstant(int64_t &Res) const;
};


} // end namespace llvm.

#endif
