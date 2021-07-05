//===-- EMBERInstPrinter.cpp - Convert EMBER MCInst to asm syntax ---------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This class prints an EMBER MCInst to a .s file.
//
//===----------------------------------------------------------------------===//

#include "EMBERInstPrinter.h"
#include "EMBERBaseInfo.h"
#include "EMBERMCExpr.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include <sstream>

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "EMBERGenAsmWriter.inc"

void EMBERInstPrinter::printInst(const MCInst          *MI, 
                                 uint64_t               Address,
                                 StringRef              Annot, 
                                 const MCSubtargetInfo &STI,
                                 raw_ostream           &O) 
{
    const MCInst *NewMI = MI;
    printInstruction(NewMI, Address, STI, O);
    printAnnotation(O, Annot);
}

void EMBERInstPrinter::printRegName(raw_ostream &O, unsigned RegNo) const 
{
    O << getRegisterName(RegNo);
}

void EMBERInstPrinter::printOperand(const MCInst          *MI, 
                                    unsigned               OpNo,
                                    const MCSubtargetInfo &STI, 
                                    raw_ostream           &O,
                                    const char            *Modifier) 
{
    assert((Modifier == 0 || Modifier[0] == 0) && "No modifiers supported");
    const MCOperand &MO = MI->getOperand(OpNo);

    if (MO.isReg()) 
    {
        printRegName(O, MO.getReg());
        return;
    }

    if (MO.isImm())
    {
        // Write the constant as hex (we could look at the opcode and determine how to write, how many digits, etc.)
        std::stringstream value;
        value << "$" << std::hex << (uint32_t)MO.getImm();

        O << value.str();
        return;
    }

    assert(MO.isExpr() && "Unknown operand kind in printOperand");
    MO.getExpr()->print(O, &MAI);
}

void EMBERInstPrinter::printBranchOperand(const MCInst          *MI,
                                          uint64_t               Address,
                                          unsigned               OpNo,
                                          const MCSubtargetInfo &STI,
                                          raw_ostream           &O)
{
    const MCOperand &MO = MI->getOperand(OpNo);
    if (!MO.isImm())
        return printOperand(MI, OpNo, STI, O);

    if (PrintBranchImmAsAddress) 
    {
        uint64_t Target = Address + MO.getImm();
        O << formatHex(Target);
    }
    else 
    {
        O << MO.getImm();
    }
}

