//===-- EMBERFixupKinds.h - EMBER Specific Fixup Entries --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERFIXUPKINDS_H
#define LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

#undef EMBER

namespace llvm 
{
namespace EMBER 
{
enum Fixups 
{
    fixup_ember_branch = FirstTargetFixupKind,
    fixup_ember_label_addr,
    fixup_ember_ldi_label_addr_lo,
    fixup_ember_ldi_label_addr_hi,

    // fixup_riscv_invalid - used as a sentinel and a marker, must be last fixup
    fixup_ember_invalid,
    NumTargetFixupKinds = fixup_ember_invalid - FirstTargetFixupKind
};
} // end namespace EMBER
} // end namespace llvm

#endif
