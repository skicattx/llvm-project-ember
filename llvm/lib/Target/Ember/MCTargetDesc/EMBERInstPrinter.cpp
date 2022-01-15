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
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

#include <sstream>
#include <iomanip>

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

    bool bHandleAddress = false;
    switch (MI->getOpcode()) 
    {
        default:
            break;
        case EMBER::ST_w_a:
        case EMBER::ST_h_a:
        case EMBER::ST_sh_a:
        case EMBER::ST_b_a:
        case EMBER::ST_sb_a:
        case EMBER::ST_hh_a:
        case EMBER::ST_bb_a:
        case EMBER::ST_bbbb_a:
        case EMBER::ST_w_ao:
        case EMBER::ST_h_ao:
        case EMBER::ST_sh_ao:
        case EMBER::ST_b_ao:
        case EMBER::ST_sb_ao:
        case EMBER::ST_hh_ao:
        case EMBER::ST_bb_ao:
        case EMBER::ST_bbbb_ao:
            bHandleAddress = (OpNo==0);
            if (OpNo == 1 && MO.isImm())
                return; // skip imm, we've already printed it
            break;
        case EMBER::LD_w_a:
        case EMBER::LD_h_a:
        case EMBER::LD_sh_a:
        case EMBER::LD_b_a:
        case EMBER::LD_sb_a:
        case EMBER::LD_hh_a:
        case EMBER::LD_bb_a:
        case EMBER::LD_bbbb_a:
        case EMBER::LD_w_ao:
        case EMBER::LD_h_ao:
        case EMBER::LD_sh_ao:
        case EMBER::LD_b_ao:
        case EMBER::LD_sb_ao:
        case EMBER::LD_hh_ao:
        case EMBER::LD_bb_ao:
        case EMBER::LD_bbbb_ao:
            bHandleAddress = (OpNo==1);
            if (OpNo == 2 && MO.isImm())
                return; // skip imm, we've already printed it
            break;
    }

    if (MO.isReg()) 
    {
        if (bHandleAddress)
            O << "(";

        printRegName(O, MO.getReg());

        if (bHandleAddress) 
        {
            if (MI->getNumOperands() > OpNo + 1) 
            {
                const MCOperand &MOff = MI->getOperand(OpNo+1);
                if (MOff.isImm()) 
                {
                    int16_t offset = MOff.getImm();
                    if (offset >= 0)
                      O << "+";

                    O << (int16_t)offset;
                }
            }

            O << ")";
        }

        return;
    }

    if (MO.isImm())
    {
        // Write the constant as hex (we could look at the opcode and determine how to write, how many digits, etc.)
        std::stringstream value;
        value << "$" << std::hex;

        switch (MI->getOpcode()) 
        {
            // > 16-bit values, or unknown
            default:          
                value << (uint32_t)MO.getImm();
                break;

            // 16-bit Values
            case EMBER::LDI_w_lo:
            case EMBER::LDI_h_lo:
            case EMBER::LDIS_sh:
            case EMBER::LDI_bb_lo:
                value << std::setfill('0') << std::setw(4) << (uint16_t)MO.getImm();
                break;

            // 8-bit Values
            case EMBER::LDI_b_lo:
            case EMBER::LDIS_sb:
                value << (uint8_t)MO.getImm();
                break;
        }

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

