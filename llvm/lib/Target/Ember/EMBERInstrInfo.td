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


defm ADD : EMBERInstr_rr_ri<0x0A, "add", [], NoItinerary>;
defm SUB : EMBERInstr_rr_ri<0x0B, "sub", [], NoItinerary>;

