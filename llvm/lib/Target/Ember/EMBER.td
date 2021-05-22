//===-- EMBER.td - Describe the EMBER Target Machine -------*- tablegen -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// EMBER Subtarget features.
//

//===----------------------------------------------------------------------===//
// Register File, Calling Conv, Instruction Descriptions
//===----------------------------------------------------------------------===//

include "EMBERRegisterInfo.td"
//include "EMBERCallingConv.td"
//include "EMBERSchedule.td"
include "EMBERInstrInfo.td"

def EMBERInstrInfo : InstrInfo;

def EMBERAsmParser : AsmParser {
  bit ShouldEmitMatchRegisterName = 0;
}

//===----------------------------------------------------------------------===//
// EMBER processors supported.
//===----------------------------------------------------------------------===//

class Proc<string Name, list<SubtargetFeature> Features>
 : Processor<Name, NoItineraries, Features>;

def : Proc<"ember32",         []>;


//===----------------------------------------------------------------------===//
// Declare the target which we are implementing
//===----------------------------------------------------------------------===//

def EMBERAsmWriter : AsmWriter {
  string AsmWriterClassName  = "InstPrinter";
  int PassSubtarget = 1;
  int Variant = 0;
}

def EMBER : Target {
  // Pull in Instruction Info:
  let InstructionSet = EMBERInstrInfo;
  let AssemblyParsers  = [EMBERAsmParser];
  let AssemblyWriters = [EMBERAsmWriter];
  let AllowRegisterRenaming = 1;
}
