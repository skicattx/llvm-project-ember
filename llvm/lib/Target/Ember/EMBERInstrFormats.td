//======================================================================================================================
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//======================================================================================================================

// Format specifies the encoding used by the instruction.  This is part of the
// ad-hoc solution used to emit machine instruction encodings by our machine
// code emitter.
class Format<bits<4> val> 
{
    bits<4> Value = val;
}

//def FmtPseudo : Format<0>;  // Virtual Instruction? (maps to another with different bits?)
//def FmtALU    : Format<1>;  // add, aub, lsr, etc.
//def FmtMOV    : Format<2>;  // mov
//def FmtOther  : Format<15>; // Instruction w/ a custom format


//======================================================================================================================
// EMBER32 Base Instruction Format
//======================================================================================================================
class EMBERInst<bits<6> op, bits<3> cond, dag outs, dag ins, string asmstr, list<dag> pattern, InstrItinClass itin> : 
    Instruction
{
    field bits<32> Inst;
    field bits<32> SoftFail; //?

    // Top 6 bits are the 'opcode' field
    let Inst{31-26} = op;
    // Then 3 bits for the 'Condcode' field
    let Inst{25-23} = cond;
    // Other bits depend on the various opcodes

    let OutOperandList = outs;
    let InOperandList  = ins;

    // ?
    let AsmString   = asmstr;
    let Pattern     = pattern;
    let Itinerary   = itin;

    let Namespace = "EMBER";
    let DecoderNamespace = "EMBER";
    let Size = 4;
}


//======================================================================================================================
// Covers Most of the EMBER OpCodes with Dest Register and at least one Source Register : 
//  <|opcode|cond|rd|ra|*|>
//======================================================================================================================
class EMBERInstr_rd_ra<bits<6> op, bits<3> cond, dag outs, dag ins, string opcodecondstr, string operandstr, list<dag> pattern, InstrItinClass itin> :
    EMBERInst<op, cond, outs, ins, opcodecondstr # operandstr, pattern, itin>
{
    bits<4>  rd;      // Dest Reg
    bits<4>  ra;      // Src A Reg

    // 4 bits for Dest Reg
    let Inst{22-19} = rd;

    // 4 bits for Src A Reg
    let Inst{18-15} = ra;
}


//======================================================================================================================
// EMBER OpCodes with Dest Register, Source Register, and either second Source Register OR 14-bit unsigned Immediate :
//  <|opcode|cond|rd|ra|fl|rd/uimm14|>
//======================================================================================================================
multiclass EMBERInstr_rr_ri<bits<6> op, bits<3> cond, string opcodestr, list<dag> pattern, InstrItinClass itin>
{
    def _rr : EMBERInstr_rd_ra<op, cond, (outs GPR32:$rd), (ins GPR32:$ra, GPR32:$rb), opcodestr, "\t$rd, $ra, $rb", pattern, itin>
    {
        bits<4>  rb;        // Src B Reg

        // 1 bit for flag
        let Inst{14} = 0;   // Register flag

        // 4 bits for Src B Reg
        let Inst{13-10} = ra;
        let Inst{9-0} = 0;
    }
    def _ri : EMBERInstr_rd_ra<op, cond, (outs GPR32:$rd), (ins GPR32:$ra, uimm14:$imm14), opcodestr, "\t$rd, $ra, $imm14", pattern, itin>
    {
        bits<14>  imm14;

        // 1 bit for flag
        let Inst{14} = 1;   // Immediate flag

        // 14 bits for Src Immediate Value
        let Inst{13-0} = imm14;
    }
}


//======================================================================================================================
// Add in the condition codes and aliases
//======================================================================================================================
multiclass EMBERInstr_rd_ra_rb_imm14<bits<6> op, string opcodestr, list<dag> pattern, InstrItinClass itin>
{
    defm _al : EMBERInstr_rr_ri<op, 0x0, opcodestr, pattern, itin>;
                                         
    defm _eq : EMBERInstr_rr_ri<op, 0x1, opcodestr # ".eq", pattern, itin>; 
    def : MnemonicAlias<opcodestr # ".z", opcodestr # ".eq">;
                                         
    defm _ne : EMBERInstr_rr_ri<op, 0x2, opcodestr # ".ne", pattern, itin>;
    def : MnemonicAlias<opcodestr # ".nz", opcodestr # ".ne">;
                                         
    defm _ng : EMBERInstr_rr_ri<op, 0x3, opcodestr # ".lt", pattern, itin>;
    def : MnemonicAlias<opcodestr # ".ng", opcodestr # ".lt">;
                                         
    defm _ge : EMBERInstr_rr_ri<op, 0x4, opcodestr # ".ge", pattern, itin>;
    def : MnemonicAlias<opcodestr # ".p", opcodestr # ".ge">;
                                         
    defm _c  : EMBERInstr_rr_ri<op, 0x5, opcodestr # ".c" , pattern, itin>;
    defm _nc : EMBERInstr_rr_ri<op, 0x6, opcodestr # ".nc", pattern, itin>;
    defm _v  : EMBERInstr_rr_ri<op, 0x7, opcodestr # ".v" , pattern, itin>;
}

