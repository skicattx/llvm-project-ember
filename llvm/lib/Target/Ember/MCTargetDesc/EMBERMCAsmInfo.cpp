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
//   Data16bitsDirective = "\t.half\t";
//   Data32bitsDirective = "\t.word\t";


    DollarIsPC = true;
    SeparatorString = "\n";
//  AllowQuestionAtStartOfIdentifier = true;
    AllowDollarAtStartOfIdentifier = true;
//  AllowAtAtStartOfIdentifier = true;
    UseMotorolaIntegers = true;
}

// const MCExpr *EMBERMCAsmInfo::getExprForFDESymbol(const MCSymbol *Sym,
//                                                   unsigned Encoding,
//                                                   MCStreamer &Streamer) const {
//   if (!(Encoding & dwarf::DW_EH_PE_pcrel))
//     return MCAsmInfo::getExprForFDESymbol(Sym, Encoding, Streamer);
// 
//   // The default symbol subtraction results in an ADD/SUB relocation pair.
//   // Processing this relocation pair is problematic when linker relaxation is
//   // enabled, so we follow binutils in using the R_EMBER_32_PCREL relocation
//   // for the FDE initial location.
//   MCContext &Ctx = Streamer.getContext();
//   const MCExpr *ME =
//       MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, Ctx);
//   assert(Encoding & dwarf::DW_EH_PE_sdata4 && "Unexpected encoding");
//   return EMBERMCExpr::create(ME, EMBERMCExpr::VK_EMBER_32_PCREL, Ctx);
// }
