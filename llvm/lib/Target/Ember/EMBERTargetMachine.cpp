//===-- EMBERTargetMachine.cpp - Define TargetMachine for EMBER -----------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "EMBERTargetMachine.h"
//#include "LeonPasses.h"
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
/*
const EMBERSubtarget *
EMBERTargetMachine::getSubtargetImpl(const Function &F) const {
  Attribute CPUAttr = F.getFnAttribute("target-cpu");
  Attribute FSAttr = F.getFnAttribute("target-features");

  std::string CPU =
      CPUAttr.isValid() ? CPUAttr.getValueAsString().str() : TargetCPU;
  std::string FS =
      FSAttr.isValid() ? FSAttr.getValueAsString().str() : TargetFS;

  // FIXME: This is related to the code below to reset the target options,
  // we need to know whether or not the soft float flag is set on the
  // function, so we can enable it as a subtarget feature.
  bool softFloat = F.getFnAttribute("use-soft-float").getValueAsBool();

  if (softFloat)
    FS += FS.empty() ? "+soft-float" : ",+soft-float";

  auto &I = SubtargetMap[CPU + FS];
  if (!I) {
    // This needs to be done before we create a new subtarget since any
    // creation will depend on the TM and the code generation flags on the
    // function that reside in TargetOptions.
    resetTargetOptions(F);
    I = std::make_unique<EMBERSubtarget>(TargetTriple, CPU, FS, *this);
  }
  return I.get();
}
*/
namespace {
/// EMBER Code Generator Pass Configuration Options.
class EMBERPassConfig : public TargetPassConfig {
public:
  EMBERPassConfig(EMBERTargetMachine &TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  EMBERTargetMachine &getEMBERTargetMachine() const {
    return getTM<EMBERTargetMachine>();
  }

  void addIRPasses() override;
//   bool addInstSelector() override;
//   void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *EMBERTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new EMBERPassConfig(*this, PM);
}

void EMBERPassConfig::addIRPasses() {
  addPass(createAtomicExpandPass());

  TargetPassConfig::addIRPasses();
}

// bool EMBERPassConfig::addInstSelector() {
//   addPass(createEMBERISelDag(getEMBERTargetMachine()));
//   return false;
// }

// void EMBERPassConfig::addPreEmitPass(){
//   addPass(createEMBERDelaySlotFillerPass());
// 
//   if (this->getEMBERTargetMachine().getSubtargetImpl()->insertNOPLoad())
//   {
//     addPass(new InsertNOPLoad());
//   }
//   if (this->getEMBERTargetMachine().getSubtargetImpl()->detectRoundChange()) {
//     addPass(new DetectRoundChange());
//   }
//   if (this->getEMBERTargetMachine().getSubtargetImpl()->fixAllFDIVSQRT())
//   {
//     addPass(new FixAllFDIVSQRT());
//   }
// }

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

