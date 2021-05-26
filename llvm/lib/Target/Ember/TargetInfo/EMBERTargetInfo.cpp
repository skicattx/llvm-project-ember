//===-- EMBERTargetInfo.cpp - EMBER Target Implementation -----------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#include "TargetInfo/EMBERTargetInfo.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

Target &llvm::getTheEMBER32Target() 
{
    static Target TheEMBER32Target;
    return TheEMBER32Target;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEMBERTargetInfo()
{
    RegisterTarget<Triple::ember> X(getTheEMBER32Target(), "ember", "32-bit Ember", "EMBER");
}
