//===-- EMBER.h - Top-level interface for EMBER representation --*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// EMBER back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_EMBER_H
#define LLVM_LIB_TARGET_EMBER_EMBER_H

#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm 
{
    class FunctionPass;
    class EMBERTargetMachine;
    class AsmPrinter;
    class MCInst;
    class MachineInstr;
} // end namespace llvm;

#endif
