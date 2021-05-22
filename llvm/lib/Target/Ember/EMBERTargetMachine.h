//===-- EMBERTargetMachine.h - Define TargetMachine for EMBER ---*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file declares the EMBER specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_EMBERTARGETMACHINE_H
#define LLVM_LIB_TARGET_EMBER_EMBERTARGETMACHINE_H

//#include "EMBERInstrInfo.h"
#include "EMBERSubtarget.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class EMBERTargetMachine : public LLVMTargetMachine {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  EMBERSubtarget Subtarget;
  mutable StringMap<std::unique_ptr<EMBERSubtarget>> SubtargetMap;
public:
  EMBERTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                     StringRef FS, const TargetOptions &Options,
                     Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                     CodeGenOpt::Level OL, bool JIT/*, bool is64bit*/);
  ~EMBERTargetMachine() override;

  const EMBERSubtarget *getSubtargetImpl() const { return &Subtarget; }
  const EMBERSubtarget *getSubtargetImpl(const Function &) const override;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

/// EMBER 32-bit target machine
///
class EMBER32TargetMachine : public EMBERTargetMachine {
  virtual void anchor();
public:
  EMBER32TargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Optional<Reloc::Model> RM, Optional<CodeModel::Model> CM,
                       CodeGenOpt::Level OL, bool JIT);
};

} // end namespace llvm

#endif
