//===-- EMBERRegisterInfo.td - EMBER Register defs -----------*- tablegen -*-===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the EMBER register file
//===----------------------------------------------------------------------===//
let Namespace = "EMBER" in {
def gpr32     : SubRegIndex<16>;
def sys32     : SubRegIndex<64>;
}

class Unallocatable {
  bit isAllocatable = 0;
}

// Most instructions use banks of 16 registers each.
class EMBERReg<bits<4> Enc, string n> : Register<n> {
  let HWEncoding{3-0} = Enc;
  let Namespace = "EMBER";
}

// MOV instructions can use two banks of 19(32) registers each.
class EMBERSysReg<bits<6> Enc, string n> : Register<n> {
  let HWEncoding{5-0} = Enc;
  let Namespace = "EMBER";
}

// EMBER CPU Registers.
class EMBERGPRReg<bits<4> Enc, string n> : EMBERReg<Enc, n>;

// EMBER SYS Registers
class EMBERMovReg<bits<6> Enc, string n> : EMBERSysReg<Enc, n>;


//===----------------------------------------------------------------------===//
//  Registers
//===----------------------------------------------------------------------===//

let Namespace = "EMBER" in {

  // General Purpose Registers 
  def R0  : EMBERGPRReg< 0,  "r0">,  DwarfRegNum<[0]>;
  def R1  : EMBERGPRReg< 1,  "r1">,  DwarfRegNum<[1]>;
  def R2  : EMBERGPRReg< 2,  "r2">,  DwarfRegNum<[2]>;
  def R3  : EMBERGPRReg< 3,  "r3">,  DwarfRegNum<[3]>;
  def R4  : EMBERGPRReg< 4,  "r4">,  DwarfRegNum<[4]>;
  def R5  : EMBERGPRReg< 5,  "r5">,  DwarfRegNum<[5]>;
  def R6  : EMBERGPRReg< 6,  "r6">,  DwarfRegNum<[6]>;
  def R7  : EMBERGPRReg< 7,  "r7">,  DwarfRegNum<[7]>;
  def R8  : EMBERGPRReg< 8,  "r8">,  DwarfRegNum<[8]>;
  def R9  : EMBERGPRReg< 9,  "r9">,  DwarfRegNum<[9]>;
  def R10 : EMBERGPRReg< 10, "r10">, DwarfRegNum<[10]>;
  def R11 : EMBERGPRReg< 11, "r11">, DwarfRegNum<[11]>;
  def R12 : EMBERGPRReg< 12, "r12">, DwarfRegNum<[12]>;
  def R13 : EMBERGPRReg< 13, "r13">, DwarfRegNum<[13]>;
  def LR  : EMBERGPRReg< 14, "lr">,  DwarfRegNum<[14]>;
  def SP  : EMBERGPRReg< 15, "sp">,  DwarfRegNum<[15]>;

  // System Registers
  def CC   : EMBERMovReg< 16, "cc">,  DwarfRegNum<[16]>;
  def PC   : EMBERMovReg< 17, "pc">,  DwarfRegNum<[17]>;
  def C0   : EMBERMovReg< 18, "c0">,  DwarfRegNum<[18]>;
  def C1   : EMBERMovReg< 19, "c1">,  DwarfRegNum<[19]>;
  def C2   : EMBERMovReg< 20, "c2">,  DwarfRegNum<[20]>;
  def C3   : EMBERMovReg< 21, "c3">,  DwarfRegNum<[21]>;

//  .. 22-31 reserved

  // 'User' General Purpose Registers - Only accessible from Supervisor Mode code
  def UR0  : EMBERMovReg< 32, "ur0">,  DwarfRegNum<[32]>;
  def UR1  : EMBERMovReg< 33, "ur1">,  DwarfRegNum<[33]>;
  def UR2  : EMBERMovReg< 34, "ur2">,  DwarfRegNum<[34]>;
  def UR3  : EMBERMovReg< 35, "ur3">,  DwarfRegNum<[35]>;
  def UR4  : EMBERMovReg< 36, "ur4">,  DwarfRegNum<[36]>;
  def UR5  : EMBERMovReg< 37, "ur5">,  DwarfRegNum<[37]>;
  def UR6  : EMBERMovReg< 38, "ur6">,  DwarfRegNum<[38]>;
  def UR7  : EMBERMovReg< 39, "ur7">,  DwarfRegNum<[39]>;
  def UR8  : EMBERMovReg< 40, "ur8">,  DwarfRegNum<[40]>;
  def UR9  : EMBERMovReg< 41, "ur9">,  DwarfRegNum<[41]>;
  def UR10 : EMBERMovReg< 42, "ur10">, DwarfRegNum<[42]>;
  def UR11 : EMBERMovReg< 43, "ur11">, DwarfRegNum<[43]>;
  def UR12 : EMBERMovReg< 44, "ur12">, DwarfRegNum<[44]>;
  def UR13 : EMBERMovReg< 45, "ur13">, DwarfRegNum<[45]>;
  def ULR  : EMBERMovReg< 46, "ulr">,  DwarfRegNum<[46]>;
  def USP  : EMBERMovReg< 47, "usp">,  DwarfRegNum<[47]>;

  // System Registers
  def UCC   : EMBERMovReg< 48, "ucc">, DwarfRegNum<[48]>;
  def UPC   : EMBERMovReg< 49, "upc">, DwarfRegNum<[49]>;
  def UC0   : EMBERMovReg< 50, "uc0">, DwarfRegNum<[50]>;
  def UC1   : EMBERMovReg< 51, "uc1">, DwarfRegNum<[51]>;
  def UC2   : EMBERMovReg< 52, "uc2">, DwarfRegNum<[52]>;
  def UC3   : EMBERMovReg< 53, "uc3">, DwarfRegNum<[53]>;

//  .. 54-63 reserved

}


//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

def GPR32 : RegisterClass<"EMBER", [i32], 32,
    ( add 
    // Reserved
    R0,
    (sequence "R%u", 1, 13),
    // Reserved
    LR, SP, PC, C0, C1, C2, C3)>;

def SYS32 : RegisterClass<"EMBER", [i32], 32,
    ( add 
    // Reserved
    R0,
    (sequence "R%u", 1, 13),
    // Reserved
    LR, SP, PC, C0, C1, C2, C3,
    // Reserved
    UR0,
    (sequence "UR%u", 1, 13),
    // Reserved
    ULR, USP, UPC, UC0, UC1, UC2, UC3)>;

def SP32 : RegisterClass<"EMBER", [i32], 32, (add SP)>;

//===----------------------------------------------------------------------===//
// Register Operands.
//===----------------------------------------------------------------------===//

class EMBERAsmRegOperand : AsmOperandClass {
  let ParserMethod = "parseRegister";
  // let EncoderMethod
  // let DecoderMethod
  // let ParserMatchClass
}

def GPR32AsmOperand : EMBERAsmRegOperand {
  let Name = "GPR32AsmReg";
  let PredicateMethod = "isGPRAsmReg";
}

def SYS32AsmOperand : EMBERAsmRegOperand {
  let Name = "SYS32AsmReg";
  let PredicateMethod = "isSYSAsmReg";
}

def SP32AsmOperand : EMBERAsmRegOperand {
  let Name = "SP32AsmReg";
  let PredicateMethod = "isSPAsmReg";
}


class EMBERAsmSImmOperand<int width> : AsmOperandClass 
{
    let Name = "SImm" # width;
    let RenderMethod = "addImmOperands";
    let DiagnosticType = !strconcat("Invalid", Name);
}

def simm14 : Operand<i32>
{
    let ParserMatchClass = EMBERAsmSImmOperand<14>;
    let EncoderMethod = "getImmOpValue";
}

class EMBERAsmUImmOperand<int width> : AsmOperandClass 
{
    let Name = "UImm" # width;
    let RenderMethod = "addImmOperands";
    let DiagnosticType = !strconcat("Invalid", Name);
}

def uimm14 : Operand<i32>
{
    let ParserMatchClass = EMBERAsmUImmOperand<14>;
    let EncoderMethod = "getImmOpValue";
}


def BranchTargetAsmOperand : AsmOperandClass 
{
    let Name = "BranchTarget";
    let ParserMethod = "parseBranchTarget";
    let RenderMethod = "addImmOperands";
    let DiagnosticType = !strconcat("Invalid", Name);
}

def brtargetSImm22 : Operand<i32>
{
  let EncoderMethod = "getBranchTargetOpValueSImm22";
  let PrintMethod = "printBranchOperand";
  let OperandType = "OPERAND_PCREL";
  let DecoderMethod = "DecodeBranchTargetSImm22";
  let ParserMatchClass = BranchTargetAsmOperand;
}
