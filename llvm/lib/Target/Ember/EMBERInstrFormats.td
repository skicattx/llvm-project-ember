//======================================================================================================================
//
// Copyright (c) 2022 IARI Ventures, LLC. All rights reserved.
//
//======================================================================================================================


//======================================================================================================================
// EMBER32 Base Instruction Format
//======================================================================================================================
class EMBERInst<bits<6> op, dag outs, dag ins, string asmstr, list<dag> pattern, InstrItinClass itin> : 
    Instruction
{
    field bits<32> Inst;
    field bits<32> SoftFail; //?

    // Top 6 bits are the 'opcode' field
    let Inst{31-26} = op;
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
//======================================================================================================================
// EMBER 'System' OpCodes with No Operands : 
//  <|opcode|>
//======================================================================================================================
class EMBERInstrSys<bits<6> op, string opcodestr, list<dag> pattern, InstrItinClass itin> :
    EMBERInst<op, (outs), (ins), opcodestr, pattern, itin>
{
    // Clear all other bits
    let Inst{25-0} = 0;
}



//======================================================================================================================
//======================================================================================================================
// EMBER Branch OpCodes with Register or Immediate value: 
//  <|opcode|cond|fl|ra/simm22|>
//======================================================================================================================
multiclass EMBERInstr_bra_r_i<bits<6> op, bits<3> cond, string opcodecondstr, list<dag> pattern, InstrItinClass itin>
{
    def _r : EMBERInst<op, (outs), (ins GPR32:$ra), opcodecondstr # "\t$ra", pattern, itin>
    {
        // Branch to address in a register
        bits<4>  ra;        // Src A Reg

        // Then 3 bits for the 'cond code' field 
        let Inst{25-23} = cond;

        // 1 bit for flag
        let Inst{22} = 0;   // Register flag

        let Inst{21-14} = 0;
        // 4 bits for Src B Reg
        let Inst{13-10} = ra;
        let Inst{9-0} = 0;
    }
    def _i : EMBERInst<op, (outs), (ins brtargetSImm22:$br22), opcodecondstr # "\t$br22", pattern, itin>
    {
        // Branch to signed immediate offset from current location
        bits<22>  br22;

        // Then 3 bits for the 'cond code' field 
        let Inst{25-23} = cond;

        // 1 bit for flag
        let Inst{22} = 1;   // Immediate flag

        // 22 bits for signed Src Immediate Branch Offset Value
        let Inst{21-0} = br22;
    }
}

//======================================================================================================================
// Add in the condition codes and aliases
//======================================================================================================================
multiclass EMBERInstr_bra_cond<bits<6> op, string opcodestr, list<dag> pattern, InstrItinClass itin>
{
    defm _al : EMBERInstr_bra_r_i<op, 0x0, opcodestr, pattern, itin>;
                                         
    defm _eq : EMBERInstr_bra_r_i<op, 0x1, opcodestr # ".eq", pattern, itin>; 
    def : MnemonicAlias<opcodestr # ".z", opcodestr # ".eq">;
                                         
    defm _ne : EMBERInstr_bra_r_i<op, 0x2, opcodestr # ".ne", pattern, itin>;
    def : MnemonicAlias<opcodestr # ".nz", opcodestr # ".ne">;
                                         
    defm _ng : EMBERInstr_bra_r_i<op, 0x3, opcodestr # ".lt", pattern, itin>;
    def : MnemonicAlias<opcodestr # ".ng", opcodestr # ".lt">;
                                         
    defm _ge : EMBERInstr_bra_r_i<op, 0x4, opcodestr # ".ge", pattern, itin>;
    def : MnemonicAlias<opcodestr # ".p", opcodestr # ".ge">;
                                         
    defm _c  : EMBERInstr_bra_r_i<op, 0x5, opcodestr # ".c" , pattern, itin>;
    defm _nc : EMBERInstr_bra_r_i<op, 0x6, opcodestr # ".nc", pattern, itin>;

    defm _v  : EMBERInstr_bra_r_i<op, 0x7, opcodestr # ".v" , pattern, itin>;
}



//======================================================================================================================
//======================================================================================================================
// EMBER LDI OpCodes with 16-bit Immediate value: 
//  <|opcode|cond|h|0|s|imm16|>
//======================================================================================================================
multiclass EMBERInstr_ldi_lohis_i<bits<6> op, bits<3> width, string widthstr, list<dag> pattern, InstrItinClass itin>
{
    def _lo : EMBERInst<op, (outs GPR32:$rd), (ins uimm32:$imm32), "ldi" # widthstr # "\t$rd, \t$imm32", pattern, itin>
    {
        bits<4>  rd;        // Dest Reg
        bits<32> imm32;     // LDI low unsigned (fill hi16 with 0)
    
        // Then 3 bits for the 'WidthCode' field (.b, .h, etc.)
        let Inst{25-23} = width;

        // 4 bits for Dest Reg
        let Inst{22-19} = rd;

        // 3 flag bits 
        let Inst{18-16} = 0b000; // |lo|0|0|

        // low 16 bits of imm32 (code will validate, and add an additional ldih if needed to handle the high bits)
        let Inst{15-0} = imm32{15-0};
    }
/*
    def _los : EMBERInst<op, (outs GPR32:$rd), (ins simm16:$simm16), "ldis" # widthstr # "\t$rd, \t$simm16", pattern, itin>
    {
        bits<4>  rd;        // Dest Reg
        bits<16> simm16;    // LDI low signed (fill hi16 with bit 15 of $simm16)
    
        // Then 3 bits for the 'WidthCode' field (.b, .h, etc.)
        let Inst{25-23} = width;

        // 4 bits for Dest Reg
        let Inst{22-19} = rd;

        // 3 flag bits 
        let Inst{18-17} = 0b00;   // |lo|0|
        let Inst{16} = signed;   // |signed|

        let Inst{18-16} = 0b001;   // |lo|0|signed|

        // 16 bits for imm16
        let Inst{15-0} = simm16;
    }
*/
    def _hi : EMBERInst<op, (outs GPR32:$rd), (ins uimm16:$imm16), "ldih" # widthstr # "\t$rd, \t$imm16", pattern, itin>
    {
        bits<4>  rd;        // Dest Reg
        bits<16> imm16;     // LDI high unsigned ((imm16<<16)|(0x0000FFFF&lo16))
    
        // Then 3 bits for the 'WidthCode' field (.b, .h, etc.)
        let Inst{25-23} = width;

        // 4 bits for Dest Reg
        let Inst{22-19} = rd;

        // 3 flag bits 
        let Inst{18-16} = 0b100; // |hi|0|0|

        // 16 bits for imm16
        let Inst{15-0} = imm16;
    }
}

//======================================================================================================================
// Add in the condition codes and aliases
//======================================================================================================================
multiclass EMBERInstr_ldi<bits<6> op, list<dag> pattern, InstrItinClass itin>
{
    defm _w    : EMBERInstr_ldi_lohis_i<op, 0x0, "",      pattern, itin>;
    defm _h    : EMBERInstr_ldi_lohis_i<op, 0x1, ".h",    pattern, itin>; 
    defm _sh   : EMBERInstr_ldi_lohis_i<op, 0x2, ".sh",   pattern, itin>;
    defm _b    : EMBERInstr_ldi_lohis_i<op, 0x3, ".b",    pattern, itin>;
    defm _sb   : EMBERInstr_ldi_lohis_i<op, 0x4, ".sb",   pattern, itin>;
    defm _hh   : EMBERInstr_ldi_lohis_i<op, 0x5, ".hh",   pattern, itin>;
    defm _bb   : EMBERInstr_ldi_lohis_i<op, 0x6, ".bb",   pattern, itin>;
    defm _bbbb : EMBERInstr_ldi_lohis_i<op, 0x7, ".bbbb", pattern, itin>;
}




//======================================================================================================================
//======================================================================================================================
// Covers Most of the EMBER OpCodes with Dest Register and at least one Source Register : 
//  <|opcode|cond|rd|ra|*|>
//======================================================================================================================
class EMBERInstrALU_rd_ra<bits<6> op, bits<3> width, dag outs, dag ins, string opcodecondstr, string operandstr, list<dag> pattern, InstrItinClass itin> :
    EMBERInst<op, outs, ins, opcodecondstr # operandstr, pattern, itin>
{
    bits<4>  rd;      // Dest Reg
    bits<4>  ra;      // Src A Reg

    // Then 3 bits for the 'WidthCode' field (.b, .h, etc.)
    let Inst{25-23} = width;

    // 4 bits for Dest Reg
    let Inst{22-19} = rd;

    // 4 bits for Src A Reg
    let Inst{18-15} = ra;
}

//======================================================================================================================
// EMBER OpCodes with Dest Register, Source Register, and either second Source Register OR 14-bit Immediate :
//  <|opcode|width|rd|ra|fl|rd/uimm14|>
//======================================================================================================================
multiclass EMBERInstrALU_rd_rr_ri<bits<6> op, bits<3> width, string opcodecondstr, list<dag> pattern, InstrItinClass itin>
{
    def _rr : EMBERInstrALU_rd_ra<op, width, (outs GPR32:$rd), (ins GPR32:$ra, GPR32:$rb), opcodecondstr, "\t$rd, $ra, $rb", pattern, itin>
    {
        bits<4>  rb;        // Src B Reg

        // 1 bit for flag
        let Inst{14} = 0;   // Register flag

        // 4 bits for Src B Reg
        let Inst{13-10} = rb;
        let Inst{9-0} = 0;
    }
    def _ri : EMBERInstrALU_rd_ra<op, width, (outs GPR32:$rd), (ins GPR32:$ra, uimm14:$imm14), opcodecondstr, "\t$rd, $ra, $imm14", pattern, itin>
    {
        bits<14>  imm14;

        // 1 bit for flag
        let Inst{14} = 1;   // Immediate flag

        // 14 bits for Src Immediate Value
        let Inst{13-0} = imm14;
    }
}

//======================================================================================================================
// Add in the width codes and aliases
//======================================================================================================================
multiclass EMBERInstr_rd_ra_rb_imm14<bits<6> op, string opcodestr, list<dag> pattern, InstrItinClass itin>
{
    defm _w    : EMBERInstrALU_rd_rr_ri<op, 0x0, opcodestr,           pattern, itin>;                                        
    defm _h    : EMBERInstrALU_rd_rr_ri<op, 0x1, opcodestr # ".h",    pattern, itin>;                                        
    defm _sh   : EMBERInstrALU_rd_rr_ri<op, 0x2, opcodestr # ".sh",   pattern, itin>;                                        
    defm _b    : EMBERInstrALU_rd_rr_ri<op, 0x3, opcodestr # ".b",    pattern, itin>;                                       
    defm _sb   : EMBERInstrALU_rd_rr_ri<op, 0x4, opcodestr # ".sb",   pattern, itin>;                                        
    defm _hh   : EMBERInstrALU_rd_rr_ri<op, 0x5, opcodestr # ".hh",   pattern, itin>;
    defm _bb   : EMBERInstrALU_rd_rr_ri<op, 0x6, opcodestr # ".bb",   pattern, itin>;
    defm _bbbb : EMBERInstrALU_rd_rr_ri<op, 0x7, opcodestr # ".bbbb", pattern, itin>;
}

//======================================================================================================================
// Covers EMBER OpCodes with NO Dest Register (implicit r0) and at least one Source Register : 
//  <|opcode|cond|r0|ra|*|>
//======================================================================================================================
class EMBERInstrALU_r0_ra<bits<6> op, bits<3> width, dag outs, dag ins, string opcodecondstr, string operandstr, list<dag> pattern, InstrItinClass itin> :
    EMBERInst<op, outs, ins, opcodecondstr # operandstr, pattern, itin>
{
    bits<4>  ra;      // Src A Reg

    // Then 3 bits for the 'WidthCode' field (.b, .h, etc.)
    let Inst{25-23} = width;

    // 4 bits for Dest Reg
    let Inst{22-19} = 0;

    // 4 bits for Src A Reg
    let Inst{18-15} = ra;
}

//======================================================================================================================
// EMBER OpCodes with NO Dest Register (implicit r0), Source Register, and either second Source Register OR 14-bit Immediate :
//  <|opcode|width|r0|ra|fl|rd/uimm14|>
//======================================================================================================================
multiclass EMBERInstrALU_r0_rr_ri<bits<6> op, bits<3> width, string opcodecondstr, list<dag> pattern, InstrItinClass itin>
{
    def _rr : EMBERInstrALU_r0_ra<op, width, (outs), (ins GPR32:$ra, GPR32:$rb), opcodecondstr, "\t$ra, $rb", pattern, itin>
    {
        bits<4>  rb;        // Src B Reg

        // 1 bit for flag
        let Inst{14} = 0;   // Register flag

        // 4 bits for Src B Reg
        let Inst{13-10} = rb;
        let Inst{9-0} = 0;
    }
    def _ri : EMBERInstrALU_r0_ra<op, width, (outs), (ins GPR32:$ra, uimm14:$imm14), opcodecondstr, "\t$ra, $imm14", pattern, itin>
    {
        bits<14>  imm14;

        // 1 bit for flag
        let Inst{14} = 1;   // Immediate flag

        // 14 bits for Src Immediate Value
        let Inst{13-0} = imm14;
    }
}

//======================================================================================================================
// Add in the width codes and aliases
//======================================================================================================================
multiclass EMBERInstr_ra_rb_imm14<bits<6> op, string opcodestr, list<dag> pattern, InstrItinClass itin>
{
    defm _w    : EMBERInstrALU_r0_rr_ri<op, 0x0, opcodestr,           pattern, itin>;                                        
    defm _h    : EMBERInstrALU_r0_rr_ri<op, 0x1, opcodestr # ".h",    pattern, itin>;                                        
    defm _sh   : EMBERInstrALU_r0_rr_ri<op, 0x2, opcodestr # ".sh",   pattern, itin>;                                        
    defm _b    : EMBERInstrALU_r0_rr_ri<op, 0x3, opcodestr # ".b",    pattern, itin>;                                       
    defm _sb   : EMBERInstrALU_r0_rr_ri<op, 0x4, opcodestr # ".sb",   pattern, itin>;                                        
    defm _hh   : EMBERInstrALU_r0_rr_ri<op, 0x5, opcodestr # ".hh",   pattern, itin>;
    defm _bb   : EMBERInstrALU_r0_rr_ri<op, 0x6, opcodestr # ".bb",   pattern, itin>;
    defm _bbbb : EMBERInstrALU_r0_rr_ri<op, 0x7, opcodestr # ".bbbb", pattern, itin>;
}





