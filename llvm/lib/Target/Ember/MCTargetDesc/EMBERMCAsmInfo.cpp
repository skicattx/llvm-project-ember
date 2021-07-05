//===-- EMBERMCAsmInfo.cpp - EMBER Asm properties -------------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the EMBERMCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "EMBERMCAsmInfo.h"
#include "MCTargetDesc/EMBERMCExpr.h"
#include "llvm/ADT/Triple.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/MC/MCStreamer.h"
using namespace llvm;

void EMBERMCAsmInfo::anchor() {}

EMBERMCAsmInfo::EMBERMCAsmInfo(const Triple &TT) 
{
    CodePointerSize = 4;
    CommentString = ";";
    AlignmentIsInBytes = false;
    SupportsDebugInformation = true;
    ExceptionsType = ExceptionHandling::None;
//   Data8bitsDirective = "\t.byte\t";
//   Data16bitsDirective = "\t.half\t";
//   Data32bitsDirective = "\t.word\t";


    DollarIsPC = true;
    SeparatorString = "\n";
    AllowDollarAtStartOfIdentifier = true;
    UseMotorolaIntegers = true;
}
