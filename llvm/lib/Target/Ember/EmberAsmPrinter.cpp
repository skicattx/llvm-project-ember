//===-- EMBERAsmPrinter.cpp - EMBER LLVM assembly writer ------------------===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format SPARC assembly language.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/EMBERInstPrinter.h"
#include "MCTargetDesc/EMBERMCExpr.h"
#include "MCTargetDesc/EMBERTargetStreamer.h"
#include "EMBER.h"
#include "EMBERInstrInfo.h"
#include "EMBERTargetMachine.h"
#include "TargetInfo/EMBERTargetInfo.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineModuleInfoImpls.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "asm-printer"
namespace {
    class EMBERAsmPrinter : public AsmPrinter 
    {
        EMBERTargetStreamer &getTargetStreamer() 
        {
            return static_cast<EMBERTargetStreamer &>(*OutStreamer->getTargetStreamer());
        }
  public:
        explicit EMBERAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer) :
            AsmPrinter(TM, std::move(Streamer)) 
        {}
     
        StringRef getPassName() const override { return "EMBER Assembly Printer"; }
     
        void printOperand(const MachineInstr *MI, int opNum, raw_ostream &OS);
        void printMemOperand(const MachineInstr *MI, int opNum, raw_ostream &OS, const char *Modifier = nullptr);
     
        void emitFunctionBodyStart() override;
        void emitInstruction(const MachineInstr *MI) override;
     
        static const char *getRegisterName(unsigned RegNo) 
        {
            return EMBERInstPrinter::getRegisterName(RegNo);
        }
     
        bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &O) override;
        bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo, const char *ExtraCode, raw_ostream &O) override;
     
//         void LowerLDIOpCode(const MachineInstr *MI, const MCSubtargetInfo &STI);
     
    };
} // end of anonymous namespace


void EMBERAsmPrinter::emitInstruction(const MachineInstr *MI) 
{
//     switch (MI->getOpcode()) 
//     {
//         default:
//             break;
//         case TargetOpcode::DBG_VALUE:
//             // FIXME: Debug Value.
//             return;
//         // For LDI opcode, determine if immediate value fits in 16 bits, or add an additional LDIH
//         case EMBER::LDI_al_lo:
//         case EMBER::LDI_c_lo:
//         case EMBER::LDI_eq_lo:
//         case EMBER::LDI_ge_lo:
//         case EMBER::LDI_nc_lo:
//         case EMBER::LDI_ne_lo:
//         case EMBER::LDI_ng_lo:
//         case EMBER::LDI_v_lo:
//             LowerLDIOpCode(MI, getSubtargetInfo());
//             return;
//     }
/*
  MachineBasicBlock::const_instr_iterator I = MI->getIterator();
  MachineBasicBlock::const_instr_iterator E = MI->getParent()->instr_end();
  do {
    MCInst TmpInst;
    LowerEMBERMachineInstrToMCInst(&*I, TmpInst, *this);
    EmitToStreamer(*OutStreamer, TmpInst);
  } while ((++I != E) && I->isInsideBundle()); // Delay slot check.
*/
}

void EMBERAsmPrinter::emitFunctionBodyStart() 
{
/*
  if (!MF->getSubtarget<EMBERSubtarget>().is64Bit())
    return;

  const MachineRegisterInfo &MRI = MF->getRegInfo();
  const unsigned globalRegs[] = { SP::G2, SP::G3, SP::G6, SP::G7, 0 };
  for (unsigned i = 0; globalRegs[i] != 0; ++i) {
    unsigned reg = globalRegs[i];
    if (MRI.use_empty(reg))
      continue;

    if  (reg == SP::G6 || reg == SP::G7)
      getTargetStreamer().emitEMBERRegisterIgnore(reg);
    else
      getTargetStreamer().emitEMBERRegisterScratch(reg);
  }
*/
}

void EMBERAsmPrinter::printOperand(const MachineInstr *MI, int opNum,
                                   raw_ostream &O) 
{
/*
  const DataLayout &DL = getDataLayout();
  const MachineOperand &MO = MI->getOperand (opNum);
  EMBERMCExpr::VariantKind TF = (EMBERMCExpr::VariantKind) MO.getTargetFlags();

#ifndef NDEBUG
  // Verify the target flags.
  if (MO.isGlobal() || MO.isSymbol() || MO.isCPI()) {
    if (MI->getOpcode() == SP::CALL)
      assert(TF == EMBERMCExpr::VK_EMBER_None &&
             "Cannot handle target flags on call address");
    else if (MI->getOpcode() == SP::SETHIi || MI->getOpcode() == SP::SETHIXi)
      assert((TF == EMBERMCExpr::VK_EMBER_HI
              || TF == EMBERMCExpr::VK_EMBER_H44
              || TF == EMBERMCExpr::VK_EMBER_HH
              || TF == EMBERMCExpr::VK_EMBER_LM
              || TF == EMBERMCExpr::VK_EMBER_TLS_GD_HI22
              || TF == EMBERMCExpr::VK_EMBER_TLS_LDM_HI22
              || TF == EMBERMCExpr::VK_EMBER_TLS_LDO_HIX22
              || TF == EMBERMCExpr::VK_EMBER_TLS_IE_HI22
              || TF == EMBERMCExpr::VK_EMBER_TLS_LE_HIX22) &&
             "Invalid target flags for address operand on sethi");
    else if (MI->getOpcode() == SP::TLS_CALL)
      assert((TF == EMBERMCExpr::VK_EMBER_None
              || TF == EMBERMCExpr::VK_EMBER_TLS_GD_CALL
              || TF == EMBERMCExpr::VK_EMBER_TLS_LDM_CALL) &&
             "Cannot handle target flags on tls call address");
    else if (MI->getOpcode() == SP::TLS_ADDrr)
      assert((TF == EMBERMCExpr::VK_EMBER_TLS_GD_ADD
              || TF == EMBERMCExpr::VK_EMBER_TLS_LDM_ADD
              || TF == EMBERMCExpr::VK_EMBER_TLS_LDO_ADD
              || TF == EMBERMCExpr::VK_EMBER_TLS_IE_ADD) &&
             "Cannot handle target flags on add for TLS");
    else if (MI->getOpcode() == SP::TLS_LDrr)
      assert(TF == EMBERMCExpr::VK_EMBER_TLS_IE_LD &&
             "Cannot handle target flags on ld for TLS");
    else if (MI->getOpcode() == SP::TLS_LDXrr)
      assert(TF == EMBERMCExpr::VK_EMBER_TLS_IE_LDX &&
             "Cannot handle target flags on ldx for TLS");
    else if (MI->getOpcode() == SP::XORri || MI->getOpcode() == SP::XORXri)
      assert((TF == EMBERMCExpr::VK_EMBER_TLS_LDO_LOX10
              || TF == EMBERMCExpr::VK_EMBER_TLS_LE_LOX10) &&
             "Cannot handle target flags on xor for TLS");
    else
      assert((TF == EMBERMCExpr::VK_EMBER_LO
              || TF == EMBERMCExpr::VK_EMBER_M44
              || TF == EMBERMCExpr::VK_EMBER_L44
              || TF == EMBERMCExpr::VK_EMBER_HM
              || TF == EMBERMCExpr::VK_EMBER_TLS_GD_LO10
              || TF == EMBERMCExpr::VK_EMBER_TLS_LDM_LO10
              || TF == EMBERMCExpr::VK_EMBER_TLS_IE_LO10 ) &&
             "Invalid target flags for small address operand");
  }
#endif


  bool CloseParen = EMBERMCExpr::printVariantKind(O, TF);

  switch (MO.getType()) {
  case MachineOperand::MO_Register:
    O << "%" << StringRef(getRegisterName(MO.getReg())).lower();
    break;

  case MachineOperand::MO_Immediate:
    O << MO.getImm();
    break;
  case MachineOperand::MO_MachineBasicBlock:
    MO.getMBB()->getSymbol()->print(O, MAI);
    return;
  case MachineOperand::MO_GlobalAddress:
    PrintSymbolOperand(MO, O);
    break;
  case MachineOperand::MO_BlockAddress:
    O <<  GetBlockAddressSymbol(MO.getBlockAddress())->getName();
    break;
  case MachineOperand::MO_ExternalSymbol:
    O << MO.getSymbolName();
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    O << DL.getPrivateGlobalPrefix() << "CPI" << getFunctionNumber() << "_"
      << MO.getIndex();
    break;
  case MachineOperand::MO_Metadata:
    MO.getMetadata()->printAsOperand(O, MMI->getModule());
    break;
  default:
    llvm_unreachable("<unknown operand type>");
  }
  if (CloseParen) O << ")";
*/
}

void EMBERAsmPrinter::printMemOperand(const MachineInstr *MI, int opNum,
                                      raw_ostream &O, const char *Modifier) 
{
  /*
  printOperand(MI, opNum, O);

  // If this is an ADD operand, emit it like normal operands.
  if (Modifier && !strcmp(Modifier, "arith")) {
    O << ", ";
    printOperand(MI, opNum+1, O);
    return;
  }

  if (MI->getOperand(opNum+1).isReg() &&
      MI->getOperand(opNum+1).getReg() == SP::G0)
    return;   // don't print "+%g0"
  if (MI->getOperand(opNum+1).isImm() &&
      MI->getOperand(opNum+1).getImm() == 0)
    return;   // don't print "+0"

  O << "+";
  printOperand(MI, opNum+1, O);
*/
}

/// PrintAsmOperand - Print out an operand for an inline asm expression.
///
bool EMBERAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                      const char *ExtraCode,
                                      raw_ostream &O) {
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1] != 0) return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    default:
      // See if this is a generic print operand
      return AsmPrinter::PrintAsmOperand(MI, OpNo, ExtraCode, O);
    case 'f':
    case 'r':
     break;
    }
  }

  printOperand(MI, OpNo, O);

  return false;
}

bool EMBERAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                            unsigned OpNo,
                                            const char *ExtraCode,
                                            raw_ostream &O) {
  if (ExtraCode && ExtraCode[0])
    return true;  // Unknown modifier

  O << '[';
  printMemOperand(MI, OpNo, O);
  O << ']';

  return false;
}

// Force static initialization.
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEMBERAsmPrinter() 
{
    RegisterAsmPrinter<EMBERAsmPrinter> X(getTheEMBER32Target());
}
