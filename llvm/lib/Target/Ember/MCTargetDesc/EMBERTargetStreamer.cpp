//===-- EMBERTargetStreamer.cpp - EMBER Target Streamer Methods -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides EMBER specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "EMBERTargetStreamer.h"
// #include "EMBERMCTargetDesc.h"
// #include "llvm/Support/FormattedStream.h"
// #include "llvm/Support/EMBERAttributes.h"

using namespace llvm;

EMBERTargetStreamer::EMBERTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

void EMBERTargetStreamer::finish()
{
    /*finishAttributeSection()*/; 
}

// void EMBERTargetStreamer::emitDirectiveOptionPush() {}
// void EMBERTargetStreamer::emitDirectiveOptionPop() {}
// void EMBERTargetStreamer::emitDirectiveOptionPIC() {}
// void EMBERTargetStreamer::emitDirectiveOptionNoPIC() {}
// void EMBERTargetStreamer::emitDirectiveOptionRVC() {}
// void EMBERTargetStreamer::emitDirectiveOptionNoRVC() {}
// void EMBERTargetStreamer::emitDirectiveOptionRelax() {}
// void EMBERTargetStreamer::emitDirectiveOptionNoRelax() {}
// void EMBERTargetStreamer::emitAttribute(unsigned Attribute, unsigned Value) {}
// void EMBERTargetStreamer::finishAttributeSection() {}
// void EMBERTargetStreamer::emitTextAttribute(unsigned Attribute,
//                                             StringRef String) {}
// void EMBERTargetStreamer::emitIntTextAttribute(unsigned Attribute,
//                                                unsigned IntValue,
//                                                StringRef StringValue) {}

/*
void EMBERTargetStreamer::emitTargetAttributes(const MCSubtargetInfo &STI) {
  if (STI.hasFeature(EMBER::FeatureRV32E))
    emitAttribute(EMBERAttrs::STACK_ALIGN, EMBERAttrs::ALIGN_4);
  else
    emitAttribute(EMBERAttrs::STACK_ALIGN, EMBERAttrs::ALIGN_16);

  std::string Arch = "rv32";
  if (STI.hasFeature(EMBER::Feature64Bit))
    Arch = "rv64";
  if (STI.hasFeature(EMBER::FeatureRV32E))
    Arch += "e1p9";
  else
    Arch += "i2p0";
  if (STI.hasFeature(EMBER::FeatureStdExtM))
    Arch += "_m2p0";
  if (STI.hasFeature(EMBER::FeatureStdExtA))
    Arch += "_a2p0";
  if (STI.hasFeature(EMBER::FeatureStdExtF))
    Arch += "_f2p0";
  if (STI.hasFeature(EMBER::FeatureStdExtD))
    Arch += "_d2p0";
  if (STI.hasFeature(EMBER::FeatureStdExtC))
    Arch += "_c2p0";
  if (STI.hasFeature(EMBER::FeatureStdExtB))
    Arch += "_b0p93";
  if (STI.hasFeature(EMBER::FeatureStdExtV))
    Arch += "_v0p10";
  if (STI.hasFeature(EMBER::FeatureExtZfh))
    Arch += "_zfh0p1";
  if (STI.hasFeature(EMBER::FeatureExtZba))
    Arch += "_zba0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbb))
    Arch += "_zbb0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbc))
    Arch += "_zbc0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbe))
    Arch += "_zbe0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbf))
    Arch += "_zbf0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbm))
    Arch += "_zbm0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbp))
    Arch += "_zbp0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbproposedc))
    Arch += "_zbproposedc0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbr))
    Arch += "_zbr0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbs))
    Arch += "_zbs0p93";
  if (STI.hasFeature(EMBER::FeatureExtZbt))
    Arch += "_zbt0p93";
  if (STI.hasFeature(EMBER::FeatureExtZvamo))
    Arch += "_zvamo0p10";
  if (STI.hasFeature(EMBER::FeatureStdExtZvlsseg))
    Arch += "_zvlsseg0p10";

  emitTextAttribute(EMBERAttrs::ARCH, Arch);
}
*/

// This part is for ascii assembly output
EMBERTargetAsmStreamer::EMBERTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS) :
    EMBERTargetStreamer(S)//, OS(OS) 
{}

/*
void EMBERTargetAsmStreamer::emitDirectiveOptionPush() {
  OS << "\t.option\tpush\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionPop() {
  OS << "\t.option\tpop\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionPIC() {
  OS << "\t.option\tpic\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionNoPIC() {
  OS << "\t.option\tnopic\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionRVC() {
  OS << "\t.option\trvc\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionNoRVC() {
  OS << "\t.option\tnorvc\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionRelax() {
  OS << "\t.option\trelax\n";
}

void EMBERTargetAsmStreamer::emitDirectiveOptionNoRelax() {
  OS << "\t.option\tnorelax\n";
}

void EMBERTargetAsmStreamer::emitAttribute(unsigned Attribute, unsigned Value) {
  OS << "\t.attribute\t" << Attribute << ", " << Twine(Value) << "\n";
}

void EMBERTargetAsmStreamer::emitTextAttribute(unsigned Attribute,
                                               StringRef String) {
  OS << "\t.attribute\t" << Attribute << ", \"" << String << "\"\n";
}

void EMBERTargetAsmStreamer::emitIntTextAttribute(unsigned Attribute,
                                                  unsigned IntValue,
                                                  StringRef StringValue) {}

void EMBERTargetAsmStreamer::finishAttributeSection() {}

*/
