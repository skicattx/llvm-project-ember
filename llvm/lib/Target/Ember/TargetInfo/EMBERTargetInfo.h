//===-- EMBERTargetInfo.h - EMBER Target Implementation ---------*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_TARGETINFO_EMBERTARGETINFO_H
#define LLVM_LIB_TARGET_EMBER_TARGETINFO_EMBERTARGETINFO_H

namespace llvm {

class Target;

Target &getTheEMBER32Target();

} // namespace llvm

#endif // LLVM_LIB_TARGET_EMBER_TARGETINFO_EMBERTARGETINFO_H
