//======================================================================================================================
//
// Copyright (c) 2022 IARI Ventures, LLC. All rights reserved.
//
//======================================================================================================================

/*
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
*/

//======================================================================================================================
// Instruction Description Header
//======================================================================================================================
include "EMBERInstrFormats.td"


//======================================================================================================================
// EMBER Operation Definitions.
//======================================================================================================================

// Special/System OpCodes
def  HALT : EMBERInstrSys<0x00, "halt", [], NoItinerary>;
def  NOP  : EMBERInstrSys<0x01, "nop", [], NoItinerary>;
def  INT  : EMBERInstrSys<0x02, "int", [], NoItinerary>;
def  RTI  : EMBERInstrSys<0x03, "rti", [], NoItinerary>;

// Branch OpCodes
defm BRA  : EMBERInstr_bra_cond<0x06, "bra", [], NoItinerary>;
defm BRL  : EMBERInstr_bra_cond<0x07, "brl", [], NoItinerary>;
// defm RET  : EMBERInstr_<0x00, "ret",  [], NoItinerary>; // Virtual [ld pc, (sp)++] Bad idea? maybe just dont support this...ld can't access pc by default... 
// defm RTL  : EMBERInstr_<0x00, "rtl",  [], NoItinerary>; // Virtual [mov pc, lr]
// defm PUSH : EMBERInstr_<0x00, "push", [], NoItinerary>; // Virtual [st --(sp), rA]
// defm POP  : EMBERInstr_<0x00, "pop",  [], NoItinerary>; // Virtual [ld rD, (sp)++]

// Move OpCode
defm MOV : EMBERInstr_mov<0x0A, [], NoItinerary>;

// LDI OpCode (+LDIH)
defm LDI : EMBERInstr_ldi<0x0B, [], NoItinerary>;

// Load/Store Memory OpCodes
// defm ST : EMBERInstr_<0x0E, "ld", [], NoItinerary>;
// defm LD : EMBERInstr_<0x0F, "st", [], NoItinerary>;

// ALU OpCodes
defm ADD  : EMBERInstr_rd_ra_rb_imm14<0x10, "add", [], NoItinerary>;
defm SUB  : EMBERInstr_rd_ra_rb_imm14<0x11, "sub", [], NoItinerary>;
defm MUL  : EMBERInstr_rd_ra_rb_imm14<0x12, "mul", [], NoItinerary>;
//defm MHS  : EMBERInstr_rd_ra_rb_imm14<0x13, "mhs", [], NoItinerary>;
//defm MHU  : EMBERInstr_rd_ra_rb_imm14<0x14, "mhu", [], NoItinerary>;
defm DIV  : EMBERInstr_rd_ra_rb_imm14<0x15, "div", [], NoItinerary>;
//defm DIVU : EMBERInstr_rd_ra_rb_imm14<0x16, "divu", [], NoItinerary>;
//defm REM  : EMBERInstr_rd_ra_rb_imm14<0x17, "rem", [], NoItinerary>;   //?
//defm REMU : EMBERInstr_rd_ra_rb_imm14<0x18, "remu", [], NoItinerary>; //?

defm AND  : EMBERInstr_rd_ra_rb_imm14<0x19, "and", [], NoItinerary>;
defm OR   : EMBERInstr_rd_ra_rb_imm14<0x1A, "or",  [], NoItinerary>;
defm XOR  : EMBERInstr_rd_ra_rb_imm14<0x1B, "xor", [], NoItinerary>;
defm LSR  : EMBERInstr_rd_ra_rb_imm14<0x1C, "lsr", [], NoItinerary>;
defm LSL  : EMBERInstr_rd_ra_rb_imm14<0x1D, "lsl", [], NoItinerary>;
defm ASR  : EMBERInstr_rd_ra_rb_imm14<0x1E, "asr", [], NoItinerary>;
defm CMP  : EMBERInstr_ra_rb_imm14<0x11, "sub", [], NoItinerary>; // Virtual [SUB r0, rA, rB/[Imm15]]
defm TST  : EMBERInstr_ra_rb_imm14<0x19, "and", [], NoItinerary>; // Virtual [AND r0, rA, rB/[Imm15]]



// TODO: FPU
// FADD  = 0x30
// FSUB  = 0x31
// FMUL  = 0x32
// FDIV  = 0x33



