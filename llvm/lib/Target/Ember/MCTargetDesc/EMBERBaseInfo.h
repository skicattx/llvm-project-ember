//===-- EMBERBaseInfo.h - Top level definitions for EMBER MC ----*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains small standalone enum definitions for the EMBER target
// useful for the compiler back-end and the MC libraries.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERBASEINFO_H
#define LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERBASEINFO_H

#include "MCTargetDesc/EMBERMCTargetDesc.h"


namespace llvm 
{

// EMBERII - This namespace holds all of the target specific flags that
// instruction info tracks. All definitions must match EMBERInstrFormats.td.
namespace EMBERII
{
enum {
  InstFormatPseudo = 0,
  InstFormatALU = 1,
  InstFormatMOV = 2,
  InstFormatOther = 15,
};

} // namespace EMBERVType

} // namespace llvm

#endif
