//===-- EMBERMCTargetDesc.h - EMBER Target Descriptions ---------*- C++ -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file provides EMBER specific target descriptions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERMCTARGETDESC_H
#define LLVM_LIB_TARGET_EMBER_MCTARGETDESC_EMBERMCTARGETDESC_H

#include "llvm/Config/config.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/DataTypes.h"
#include <memory>

namespace llvm {
class MCAsmBackend;
class MCCodeEmitter;
class MCContext;
class MCInstrInfo;
class MCObjectTargetWriter;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;

MCCodeEmitter *createEMBERMCCodeEmitter(const MCInstrInfo &MCII,
                                        const MCRegisterInfo &MRI,
                                        MCContext &Ctx);

MCAsmBackend *createEMBERAsmBackend(const Target &T, const MCSubtargetInfo &STI,
                                    const MCRegisterInfo &MRI,
                                    const MCTargetOptions &Options);

std::unique_ptr<MCObjectTargetWriter> createEMBERELFObjectWriter();
}

// Defines symbolic names for EMBER registers.
#define GET_REGINFO_ENUM
#include "EMBERGenRegisterInfo.inc"

// Defines symbolic names for EMBER instructions.
#define GET_INSTRINFO_ENUM
#include "EMBERGenInstrInfo.inc"

//#define GET_SUBTARGETINFO_ENUM
#include "EMBERGenSubtargetInfo.inc"

#endif
