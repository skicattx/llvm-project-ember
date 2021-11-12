//===-- llvm/Target/EMBERTargetObjectFile.h - EMBER Object Info ---*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_EMBERTARGETOBJECTFILE_H
#define LLVM_LIB_TARGET_EMBER_EMBERTARGETOBJECTFILE_H

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm 
{
  class EMBERTargetMachine;
  class EMBERTargetObjectFile : public TargetLoweringObjectFileELF
  {
      void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

//       MCSection *SelectSectionForGlobal(const GlobalObject *GO,
//                                         SectionKind Kind,
//                                         const TargetMachine &TM) const override;

      /// Describe a TLS variable address within debug info.
      const MCExpr* getDebugThreadLocalSymbol(const MCSymbol *Sym) const override;
  };
  } // end namespace llvm

#endif
