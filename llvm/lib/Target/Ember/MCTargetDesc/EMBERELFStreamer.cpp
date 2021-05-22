//===-- EMBERELFStreamer.cpp - EMBER ELF Target Streamer Methods ----------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file provides EMBER specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "EMBERELFStreamer.h"
#include "EMBERAsmBackend.h"
#include "EMBERBaseInfo.h"
#include "EMBERMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/LEB128.h"
//#include "llvm/Support/EMBERAttributes.h"

using namespace llvm;
/*
// This part is for ELF object output.
EMBERTargetELFStreamer::EMBERTargetELFStreamer(MCStreamer &S,
                                               const MCSubtargetInfo &STI)
    : EMBERTargetStreamer(S), CurrentVendor("riscv") {
  MCAssembler &MCA = getStreamer().getAssembler();
  const FeatureBitset &Features = STI.getFeatureBits();
  auto &MAB = static_cast<EMBERAsmBackend &>(MCA.getBackend());
  EMBERABI::ABI ABI = MAB.getTargetABI();
  assert(ABI != EMBERABI::ABI_Unknown && "Improperly initialised target ABI");

  unsigned EFlags = MCA.getELFHeaderEFlags();

  if (Features[EMBER::FeatureStdExtC])
    EFlags |= ELF::EF_EMBER_RVC;

  switch (ABI) {
  case EMBERABI::ABI_ILP32:
  case EMBERABI::ABI_LP64:
    break;
  case EMBERABI::ABI_ILP32F:
  case EMBERABI::ABI_LP64F:
    EFlags |= ELF::EF_EMBER_FLOAT_ABI_SINGLE;
    break;
  case EMBERABI::ABI_ILP32D:
  case EMBERABI::ABI_LP64D:
    EFlags |= ELF::EF_EMBER_FLOAT_ABI_DOUBLE;
    break;
  case EMBERABI::ABI_ILP32E:
    EFlags |= ELF::EF_EMBER_RVE;
    break;
  case EMBERABI::ABI_Unknown:
    llvm_unreachable("Improperly initialised target ABI");
  }

  MCA.setELFHeaderEFlags(EFlags);
}

MCELFStreamer &EMBERTargetELFStreamer::getStreamer() {
  return static_cast<MCELFStreamer &>(Streamer);
}

void EMBERTargetELFStreamer::emitDirectiveOptionPush() {}
void EMBERTargetELFStreamer::emitDirectiveOptionPop() {}
void EMBERTargetELFStreamer::emitDirectiveOptionPIC() {}
void EMBERTargetELFStreamer::emitDirectiveOptionNoPIC() {}
void EMBERTargetELFStreamer::emitDirectiveOptionRVC() {}
void EMBERTargetELFStreamer::emitDirectiveOptionNoRVC() {}
void EMBERTargetELFStreamer::emitDirectiveOptionRelax() {}
void EMBERTargetELFStreamer::emitDirectiveOptionNoRelax() {}

void EMBERTargetELFStreamer::emitAttribute(unsigned Attribute, unsigned Value) {
  setAttributeItem(Attribute, Value, /*OverwriteExisting=* /true);
}

void EMBERTargetELFStreamer::emitTextAttribute(unsigned Attribute,
                                               StringRef String) {
  setAttributeItem(Attribute, String, /*OverwriteExisting=* /true);
}

void EMBERTargetELFStreamer::emitIntTextAttribute(unsigned Attribute,
                                                  unsigned IntValue,
                                                  StringRef StringValue) {
  setAttributeItems(Attribute, IntValue, StringValue,
                    /*OverwriteExisting=* /true);
}

void EMBERTargetELFStreamer::finishAttributeSection() {
  if (Contents.empty())
    return;

  if (AttributeSection) {
    Streamer.SwitchSection(AttributeSection);
  } else {
    MCAssembler &MCA = getStreamer().getAssembler();
    AttributeSection = MCA.getContext().getELFSection(
        ".riscv.attributes", ELF::SHT_EMBER_ATTRIBUTES, 0);
    Streamer.SwitchSection(AttributeSection);

    Streamer.emitInt8(ELFAttrs::Format_Version);
  }

  // Vendor size + Vendor name + '\0'
  const size_t VendorHeaderSize = 4 + CurrentVendor.size() + 1;

  // Tag + Tag Size
  const size_t TagHeaderSize = 1 + 4;

  const size_t ContentsSize = calculateContentSize();

  Streamer.emitInt32(VendorHeaderSize + TagHeaderSize + ContentsSize);
  Streamer.emitBytes(CurrentVendor);
  Streamer.emitInt8(0); // '\0'

  Streamer.emitInt8(ELFAttrs::File);
  Streamer.emitInt32(TagHeaderSize + ContentsSize);

  // Size should have been accounted for already, now
  // emit each field as its type (ULEB or String).
  for (AttributeItem item : Contents) {
    Streamer.emitULEB128IntValue(item.Tag);
    switch (item.Type) {
    default:
      llvm_unreachable("Invalid attribute type");
    case AttributeType::Numeric:
      Streamer.emitULEB128IntValue(item.IntValue);
      break;
    case AttributeType::Text:
      Streamer.emitBytes(item.StringValue);
      Streamer.emitInt8(0); // '\0'
      break;
    case AttributeType::NumericAndText:
      Streamer.emitULEB128IntValue(item.IntValue);
      Streamer.emitBytes(item.StringValue);
      Streamer.emitInt8(0); // '\0'
      break;
    }
  }

  Contents.clear();
}

size_t EMBERTargetELFStreamer::calculateContentSize() const {
  size_t Result = 0;
  for (AttributeItem item : Contents) {
    switch (item.Type) {
    case AttributeType::Hidden:
      break;
    case AttributeType::Numeric:
      Result += getULEB128Size(item.Tag);
      Result += getULEB128Size(item.IntValue);
      break;
    case AttributeType::Text:
      Result += getULEB128Size(item.Tag);
      Result += item.StringValue.size() + 1; // string + '\0'
      break;
    case AttributeType::NumericAndText:
      Result += getULEB128Size(item.Tag);
      Result += getULEB128Size(item.IntValue);
      Result += item.StringValue.size() + 1; // string + '\0';
      break;
    }
  }
  return Result;
}

*/