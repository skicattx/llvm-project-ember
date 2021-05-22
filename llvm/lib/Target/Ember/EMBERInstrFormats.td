//===-- EMBERInstrFormats.td - EMBER Instruction Formats -----*- tablegen -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

// Format specifies the encoding used by the instruction.  This is part of the
// ad-hoc solution used to emit machine instruction encodings by our machine
// code emitter.
class Format<bits<4> val> {
  bits<4> Value = val;
}

def FmtPseudo : Format<0>;  // Virtual Instruction? (maps to another with different bits?)
def FmtALU    : Format<1>;  // add, aub, lsr, etc.
def FmtMOV    : Format<2>;  // mov
def FmtOther  : Format<15>; // Instruction w/ a custom format


// EMBER32 Base Instruction Format
class EMBERInst<bits<6> op, dag outs, dag ins, string asmstr, list<dag> pattern, InstrItinClass itin> : Instruction
{
  field bits<32> Inst;
  field bits<32> SoftFail; //?

  bits<3> cond = 0; 

  let OutOperandList = outs;
  let InOperandList  = ins;

  // ?
  let AsmString   = asmstr;
  let Pattern     = pattern;
  let Itinerary   = itin;




  let Namespace = "EMBER";
  let DecoderNamespace = "EMBER";
  let Size = 4;



  // Top 6 bits are the 'opcode' field
  let Inst{31-26} = op;
  // Then 3 bits for the 'Condcode' field
  let Inst{25-23} = cond;
  // Other bits depend on the various opcodes

}

//===----------------------------------------------------------------------===//
// Format 1 Instruction class in EMBER : <|opcode|cond|rd|ra|f|rb/Imm|>
//===----------------------------------------------------------------------===//

class EMBERInstr_rd_ra_fl1<bits<6> op, bit flag, dag outs, dag ins, string asmstr, list<dag> pattern, InstrItinClass itin> :
  EMBERInst<op, outs, ins, asmstr, pattern, itin>
{
  bits<4>  rd;      // Dest Reg
  bits<4>  ra;      // Src A Reg

  // 4 bits for Dest Reg
  let Inst{22-19} = rd;

  // 4 bits for Src A Reg
  let Inst{18-15} = ra;

  // 1 bit for flag
  let Inst{14} = flag;  // 1-bit flag, varies by opcode (e.g. Imm/Reg, Signed/Unsigned, etc.)
}


multiclass EMBERInstr_rr_ri<bits<6> op, string opcodestr, list<dag> pattern, InstrItinClass itin>
{
  def _rr : EMBERInstr_rd_ra_fl1<op, 0, (outs GPR32:$rd), (ins GPR32:$ra, GPR32:$rb), opcodestr # "\t$rd, $ra, $rb", pattern, itin>
  {
      bits<4>  rb;      // Src B Reg

      // 4 bits for Src B Reg
      let Inst{13-10} = ra;
      let Inst{9-0} = 0;
  }
  def _ri : EMBERInstr_rd_ra_fl1<op, 1, (outs GPR32:$rd), (ins GPR32:$ra, simm14:$imm14), opcodestr # "\t$rd, $ra, $imm14", pattern, itin>
  {
      bits<14>  imm14;      // TODO: Need 140bit immediate value definition

      // 14 bits for Src Immediate Value
      let Inst{13-0} = imm14;
  }
}
