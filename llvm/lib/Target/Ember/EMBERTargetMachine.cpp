//===-- EMBERTargetMachine.cpp - Define TargetMachine for EMBER -----------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "EMBERTargetMachine.h"
#include "EMBER.h"
#include "EMBERTargetObjectFile.h"
#include "TargetInfo/EMBERTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"
using namespace llvm;

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEMBERTarget() 
{
    // Register the target.
    RegisterTargetMachine<EMBER32TargetMachine> X(getTheEMBER32Target());
}

static std::string computeDataLayout(const Triple &T)
{
  // EMBER is typically big endian, but some are little.
  std::string Ret = T.getArch() == Triple::sparcel ? "e" : "E";
  Ret += "-m:e";

  // Some ABIs have 32bit pointers.
  Ret += "-p:32:32";

  // Alignments for 64 bit integers.
  Ret += "-i64:64";

  return Ret;
}

static Reloc::Model getEffectiveRelocModel(Optional<Reloc::Model> RM)
{
  return RM.getValueOr(Reloc::Static);
}

static CodeModel::Model getEffectiveEMBERCodeModel(Optional<CodeModel::Model> CM, Reloc::Model RM, bool JIT) 
{
  if (CM) {
    if (*CM == CodeModel::Tiny)
      report_fatal_error("Target does not support the tiny CodeModel", false);
    if (*CM == CodeModel::Kernel)
      report_fatal_error("Target does not support the kernel CodeModel", false);
    return *CM;
  }
  return CodeModel::Small;
}

EMBERTargetMachine::EMBERTargetMachine(
    const Target &T, const Triple &TT, StringRef CPU, StringRef FS,
    const TargetOptions &Options, Optional<Reloc::Model> RM,
    Optional<CodeModel::Model> CM, CodeGenOpt::Level OL, bool JIT)
    : LLVMTargetMachine(T, computeDataLayout(TT), TT, CPU, FS, Options,
                        getEffectiveRelocModel(RM),
                        getEffectiveEMBERCodeModel(CM, getEffectiveRelocModel(RM), JIT),
                        OL),
      TLOF(std::make_unique<EMBERTargetObjectFile>()),
      Subtarget(TT, std::string(CPU), std::string(FS), *this)
{
  initAsmInfo();
}

EMBERTargetMachine::~EMBERTargetMachine() {}

namespace 
{
/// EMBER Code Generator Pass Configuration Options.
class EMBERPassConfig : public TargetPassConfig {
public:
  EMBERPassConfig(EMBERTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  EMBERTargetMachine &getEMBERTargetMachine() const {
    return getTM<EMBERTargetMachine>();
  }

  void addIRPasses() override;
};
} // namespace

TargetPassConfig *EMBERTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new EMBERPassConfig(*this, PM);
}

void EMBERPassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());

  TargetPassConfig::addIRPasses();
}


void EMBER32TargetMachine::anchor() { }

EMBER32TargetMachine::EMBER32TargetMachine(const Target                &T, 
                                           const Triple                &TT, 
                                           StringRef                    CPU, 
                                           StringRef                    FS,
                                           const TargetOptions         &Options,
                                           Optional<Reloc::Model>       RM,
                                           Optional<CodeModel::Model>   CM,
                                           CodeGenOpt::Level            OL,
                                           bool                         JIT) :
    EMBERTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, JIT) 
{}

