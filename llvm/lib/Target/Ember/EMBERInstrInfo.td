//===- EMBERInstrInfo.td - Target Description for EMBER Target -*- tablegen -*-=//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains the EMBER implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// EMBER profiles and nodes
//===----------------------------------------------------------------------===//
/*
*/

//===----------------------------------------------------------------------===//

//class EMBERPat<dag pattern, dag result> : Pat<pattern, result>, PredicateControl;

//class EMBERInstAlias<string Asm, dag Result, bit Emit = 0b1> :
//  InstAlias<Asm, Result, Emit>, PredicateControl;

class IsCommutable {
  bit isCommutable = 1;
}

class IsBranch {
  bit isBranch = 1;
  bit isCTI = 1;
}

class IsReturn {
  bit isReturn = 1;
  bit isCTI = 1;
}

class IsCall {
  bit isCall = 1;
  bit isCTI = 1;
}

class IsTailCall {
  bit isCall = 1;
  bit isTerminator = 1;
  bit isReturn = 1;
  bit isBarrier = 1;
  bit hasExtraSrcRegAllocReq = 1;
  bit isCodeGenOnly = 1;
  bit isCTI = 1;
}

class IsAsCheapAsAMove {
  bit isAsCheapAsAMove = 1;
}

class NeverHasSideEffects {
  bit hasSideEffects = 0;
}

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

include "EMBERInstrFormats.td"

//===----------------------------------------------------------------------===//
// EMBER Operand, Complex Patterns and Transformations Definitions.
//===----------------------------------------------------------------------===//

// ALU OpCodes
defm ADD : EMBERInstr_rd_ra_rb_imm14<0x0A, "add", [], NoItinerary>;
defm AND : EMBERInstr_rd_ra_rb_imm14<0x11, "and", [], NoItinerary>;
defm ASR : EMBERInstr_rd_ra_rb_imm14<0x16, "asr", [], NoItinerary>;
defm DIV : EMBERInstr_rd_ra_rb_imm14<0x0F, "div", [], NoItinerary>;
defm LSL : EMBERInstr_rd_ra_rb_imm14<0x15, "lsl", [], NoItinerary>;
defm LSR : EMBERInstr_rd_ra_rb_imm14<0x14, "lsr", [], NoItinerary>;
defm MUL : EMBERInstr_rd_ra_rb_imm14<0x0C, "mul", [], NoItinerary>;
defm OR  : EMBERInstr_rd_ra_rb_imm14<0x12, "or",  [], NoItinerary>;
defm SUB : EMBERInstr_rd_ra_rb_imm14<0x0B, "sub", [], NoItinerary>;
defm XOR : EMBERInstr_rd_ra_rb_imm14<0x13, "xor", [], NoItinerary>;

