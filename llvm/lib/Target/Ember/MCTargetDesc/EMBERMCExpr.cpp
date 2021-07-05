//===-- EMBERMCExpr.cpp - EMBER specific MC expression classes ------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the assembly expression modifiers
// accepted by the EMBER architecture (e.g. ":lo12:", ":gottprel_g1:", ...).
//
//===----------------------------------------------------------------------===//

#include "EMBERMCExpr.h"
#include "MCTargetDesc/EMBERAsmBackend.h"
#include "EMBERFixupKinds.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;


bool EMBERMCExpr::evaluateAsConstant(int64_t &Res) const 
{
    MCValue Value;

    if (Kind == VK_EMBER_PCREL_HI || Kind == VK_EMBER_PCREL_LO)
        return false;

    if (!getSubExpr()->evaluateAsRelocatable(Value, nullptr, nullptr))
        return false;

    if (!Value.isAbsolute())
        return false;

    return true;
}
