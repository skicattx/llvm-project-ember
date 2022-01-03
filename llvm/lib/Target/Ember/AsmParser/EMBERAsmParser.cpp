//===-- EMBERAsmParser.cpp - Parse EMBER assembly to MCInst instructions --===//
//
// Copyright (c) 2021 IARI Ventures, LLC. All rights reserved.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/EMBERAsmBackend.h"
#include "MCTargetDesc/EMBERBaseInfo.h"
#include "MCTargetDesc/EMBERInstPrinter.h"
#include "MCTargetDesc/EMBERMCExpr.h"
#include "MCTargetDesc/EMBERMCTargetDesc.h"
#include "MCTargetDesc/EMBERMatInt.h"
#include "MCTargetDesc/EMBERTargetStreamer.h"
#include "TargetInfo/EMBERTargetInfo.h"
#include "EMBERInstrInfo.h"
#include <string>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"

#include <limits>

using namespace llvm;

#define DEBUG_TYPE "ember-asm-parser"

namespace 
{
struct EMBEROperand;

struct ParserOptionsSet 
{
  bool IsPicEnabled;
};

class EMBERAsmParser : public MCTargetAsmParser
{
//   SmallVector<FeatureBitset, 4> FeatureBitStack;
// 
//   SmallVector<ParserOptionsSet, 4> ParserOptionsStack;
  ParserOptionsSet ParserOptions;

  SMLoc getLoc() const { return getParser().getTok().getLoc(); }

  EMBERTargetStreamer &getTargetStreamer() 
  {
      MCTargetStreamer &TS = *getParser().getStreamer().getTargetStreamer();
      return static_cast<EMBERTargetStreamer &>(TS);
  }

  bool generateImmOutOfRangeError(OperandVector &Operands, uint64_t ErrorInfo,
                                  int64_t Lower, uint64_t Upper, Twine Msg);

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  OperandMatchResultTy tryParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool ParseDirective(AsmToken DirectiveID) override;

  // Helper to actually emit an instruction to the MCStreamer. Also, when
  // possible, compression of the instruction is performed.
  void emitToStreamer(MCStreamer &S, const MCInst &Inst);

  // Emit one or two LDI/LDIH instructions depending on the size of the imm value
  void emitLDIImm(MCInst& Inst, SMLoc IDLoc, MCStreamer& Out);

  // Check instruction constraints.
  unsigned validateInstruction(MCInst &Inst, uint64_t &ErrorInfo, OperandVector &Operands);

  /// Helper for processing MC instructions that have been successfully matched
  /// by MatchAndEmitInstruction. Modifications to the emitted instructions,
  /// like the expansion of pseudo instructions can be performed in this method.
  bool processInstruction(MCInst &Inst, SMLoc IDLoc, OperandVector &Operands, MCStreamer &Out);

  // Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "EMBERGenAsmMatcher.inc"

  OperandMatchResultTy parseRegister(OperandVector &Operands);
  OperandMatchResultTy parseImmediate(OperandVector &Operands);
  OperandMatchResultTy parseBranchTarget(OperandVector& Operands);
  OperandMatchResultTy parseMemOpBaseReg(OperandVector &Operands);

  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);

public:
    enum EMBERMatchResultTy
    {
        Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "EMBERGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
    };

    EMBERAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser, const MCInstrInfo &MII, const MCTargetOptions &Options) : 
        MCTargetAsmParser(Options, STI, MII) 
    {
        Parser.addAliasForDirective(".half", ".2byte");
        Parser.addAliasForDirective(".hword", ".2byte");
        Parser.addAliasForDirective(".word", ".4byte");
        Parser.addAliasForDirective(".dword", ".8byte");

        // No feature currently, add in td file when needed
        setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
    }
};

/// EMBEROperand - Instances of this class represent a parsed machine instruction
struct EMBEROperand : public MCParsedAsmOperand
{
    enum class KindTy 
    {
        Token,
        Register,
        Immediate,
        UserRegister,
//         VType,
    } Kind;

    struct RegOp 
    {
        MCRegister RegNum;
    };

    struct UserRegOp 
    {
        MCRegister RegNum;
    };

    struct ImmOp 
    {
        const MCExpr *Val;
    };

//     struct VTypeOp 
//     {
//         unsigned Val;
//     };

    SMLoc StartLoc;
    SMLoc EndLoc;
    union
    {
        StringRef   Tok;
        RegOp       Reg;
        UserRegOp   UserReg;
        ImmOp       Imm;
//         struct VTypeOp  VType;
    };

    EMBEROperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

public:
    EMBEROperand(const EMBEROperand& o) : MCParsedAsmOperand() 
    {
        Kind = o.Kind;
        StartLoc = o.StartLoc;
        EndLoc = o.EndLoc;
        switch (Kind)
        {
            case KindTy::Register:
              Reg = o.Reg;
              break;
            case KindTy::Immediate:
              Imm = o.Imm;
              break;
            case KindTy::Token:
              Tok = o.Tok;
              break;
            case KindTy::UserRegister:
              UserReg = o.UserReg;
              break;
//             case KindTy::VType:
//               VType = o.VType;
//               break;
        }
    }

  bool isToken() const override { return Kind == KindTy::Token; }
  bool isReg() const override { return Kind == KindTy::Register; }
//   bool isV0Reg() const {
//     return Kind == KindTy::Register && Reg.RegNum == EMBER::V0;
//   }
  bool isImm() const override { return Kind == KindTy::Immediate; }
  bool isMem() const override { return false; }
  bool isSystemRegister() const { return Kind == KindTy::UserRegister; }
//   bool isVType() const { return Kind == KindTy::VType; }

  bool isGPRAsmReg() const { return Kind == KindTy::Register; /*TODO: is this one of the u registers, or sp?*/}
  bool isSYSAsmReg() const { return Kind == KindTy::Register; /*TODO: is this one of the u registers, or sp?*/}
  bool isSPAsmReg() const { return Kind == KindTy::Register; /*TODO: is this one of the u registers, or sp?*/}

    static bool evaluateConstantImm(const MCExpr *Expr, int64_t &Imm)
    {
        if (auto *RE = dyn_cast<EMBERMCExpr>(Expr)) 
        {
            return RE->evaluateAsConstant(Imm);
        }

        if (auto CE = dyn_cast<MCConstantExpr>(Expr)) 
        {
            Imm = CE->getValue();
            return true;
        }

        return false;
    }

  // True if operand is a symbol with no modifiers, or a constant with no
  // modifiers and isShiftedInt<N-1, 1>(Op).
//   template <int N> bool isBareSimmNLsb0() const {
//     int64_t Imm;
// 
//     if (!isImm())
//       return false;
//     bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
//     bool IsValid;
//     if (!IsConstantImm)
//       IsValid = false/*EMBERAsmParser::classifySymbolRef(getImm())*/;
//     else
//       IsValid = isShiftedInt<N - 1, 1>(Imm);
//     return IsValid;
//   }

  // Predicate methods for AsmOperands defined in EMBERInstrInfo.td

//   bool isBareSymbol() const {
//     int64_t Imm;
// 
//     // Must be of 'immediate' type but not a constant.
//     if (!isImm() || evaluateConstantImm(getImm(), Imm))
//       return false;
//     return false;/*EMBERAsmParser::classifySymbolRef(getImm());*/
//   }
// 
//   bool isCallSymbol() const {
//     int64_t Imm;
// 
//     // Must be of 'immediate' type but not a constant.
//     if (!isImm() || evaluateConstantImm(getImm(), Imm))
//       return false;
//     return true;
//   }
// 
//   bool isPseudoJumpSymbol() const {
//     int64_t Imm;
// 
//     // Must be of 'immediate' type but not a constant.
//     if (!isImm() || evaluateConstantImm(getImm(), Imm))
//       return false;
//     return true;
//   }


  bool isBranchTarget() const
  {
      int64_t Imm;

      // Must be of 'immediate' type but not a constant (due to relaxation, a constant offset is dangerous, so we just don't allow them).
      if (!isImm() || evaluateConstantImm(getImm(), Imm))
        return false;

      // Is it defined
//       const MCExpr* Val = getImm();
//       auto* SVal = dyn_cast<MCSymbolRefExpr>(Val);
//       if (!SVal || SVal->getSymbol().isUndefined())
//           return false;

      return true;
  }

//   bool isTPRelAddSymbol() const {
//     int64_t Imm;
// 
//     // Must be of 'immediate' type but not a constant.
//     if (!isImm() || evaluateConstantImm(getImm(), Imm))
//       return false;
//     return true;
//   }
// 
//   bool isCSRSystemRegister() const { return isSystemRegister(); }
// 
// //   bool isVTypeI() const { return isVType(); }
// 
//   /// Return true if the operand is a valid for the fence instruction e.g.
//   /// ('iorw').
//   bool isFenceArg() const {
//     if (!isImm())
//       return false;
//     const MCExpr *Val = getImm();
//     auto *SVal = dyn_cast<MCSymbolRefExpr>(Val);
//     if (!SVal || SVal->getKind() != MCSymbolRefExpr::VK_None)
//       return false;
// 
//     StringRef Str = SVal->getSymbol().getName();
//     // Letters must be unique, taken from 'iorw', and in ascending order. This
//     // holds as long as each individual character is one of 'iorw' and is
//     // greater than the previous character.
//     char Prev = '\0';
//     for (char c : Str) {
//       if (c != 'i' && c != 'o' && c != 'r' && c != 'w')
//         return false;
//       if (c <= Prev)
//         return false;
//       Prev = c;
//     }
//     return true;
//   }

  /// Return true if the operand is a valid floating point rounding mode.
/*
  bool isFRMArg() const {
    if (!isImm())
      return false;
    const MCExpr *Val = getImm();
    auto *SVal = dyn_cast<MCSymbolRefExpr>(Val);
    if (!SVal || SVal->getKind() != MCSymbolRefExpr::VK_None)
      return false;

    StringRef Str = SVal->getSymbol().getName();

    return EMBERFPRndMode::stringToRoundingMode(Str) != EMBERFPRndMode::Invalid;
  }

  bool isImmXLenLI() const {
    int64_t Imm;

    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    // Given only Imm, ensuring that the actually specified constant is either
    // a signed or unsigned 64-bit number is unfortunately impossible.
    return IsConstantImm &&
           ((isInt<32>(Imm) || isUInt<32>(Imm)));
  }

  bool isUImmLog2XLen() const {
    int64_t Imm;

    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm))
      return false;
    return (isUInt<6>(Imm)) || isUInt<5>(Imm);
  }

  bool isUImmLog2XLenNonZero() const {
    int64_t Imm;

    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm) )
      return false;
    if (Imm == 0)
      return false;
    return (isUInt<6>(Imm)) || isUInt<5>(Imm);
  }

  bool isUImmLog2XLenHalf() const {
    int64_t Imm;

    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm))
      return false;
    return (isUInt<5>(Imm)) || isUInt<4>(Imm);
  }
*/

    bool isSImm8() const 
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && (isInt<8>(Imm) || isUInt<8>(Imm));
    }
    bool isUImm8() const
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && isUInt<8>(Imm);
    }

    bool isSImm14() const 
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && isInt<14>(Imm);
    }

    bool isUImm14() const 
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && isUInt<14>(Imm);
    }

    bool isSImm16() const 
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && (isInt<16>(Imm) || isUInt<16>(Imm));
    }
    bool isUImm16() const
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && isUInt<16>(Imm);
    }


    bool isUImm22() const 
    {
        int64_t Imm;
        if (!isImm())
            return false;
        bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
        return IsConstantImm && isUInt<22>(Imm);
    }

    bool isUImm32() const 
    {
        int64_t Imm;
        if (!isImm())
            return false;

        if (evaluateConstantImm(getImm(), Imm))
        {
            // If it's a literal imm, then check the bits
            return isUInt<32>(Imm);
        }

//         const MCExpr* Val = getImm();
//         auto* SVal = dyn_cast<MCSymbolRefExpr>(Val);
//         if (!SVal || SVal->getSymbol().isUndefined())
//             return false;

        // otherwise, it's a label or whatever, and will need a fixup anyway, so just return true for now
        return true;
    }

/*  bool isUImm5() const {
    int64_t Imm;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isUInt<5>(Imm);
  }

  bool isSImm5() const {
    if (!isImm())
      return false;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isInt<5>(Imm);
  }

  bool isSImm6() const {
    if (!isImm())
      return false;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isInt<6>(Imm);
  }

  bool isSImm6NonZero() const {
    if (!isImm())
      return false;

    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isInt<6>(Imm) && (Imm != 0);

  }

  bool isCLUIImm() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && (Imm != 0);
           (isUInt<5>(Imm) || (Imm >= 0xfffe0 && Imm <= 0xfffff));

  }

  bool isUImm7Lsb00() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isShiftedUInt<5, 2>(Imm);

  }

  bool isUImm8Lsb00() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isShiftedUInt<6, 2>(Imm);

  }

  bool isUImm8Lsb000() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isShiftedUInt<5, 3>(Imm);

  }

  bool isSImm9Lsb0() const { return isBareSimmNLsb0<9>(); }

  bool isUImm9Lsb000() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isShiftedUInt<6, 3>(Imm);

  }

  bool isUImm10Lsb00NonZero() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isShiftedUInt<8, 2>(Imm) && (Imm != 0);

  }

  bool isSImm12() const {

    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    if (!IsConstantImm)
      IsValid = false/ *EMBERAsmParser::classifySymbolRef(getImm())* /;
    else
      IsValid = isInt<12>(Imm);
    return IsValid && (IsConstantImm);
  }

  bool isSImm12Lsb0() const { return isBareSimmNLsb0<12>(); }

  bool isSImm13Lsb0() const { return isBareSimmNLsb0<13>(); }

  bool isSImm10Lsb0000NonZero() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && (Imm != 0); isShiftedInt<6, 4>(Imm);

  }

  bool isUImm20LUI() const {

    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    if (!IsConstantImm) {
      IsValid = false/ *EMBERAsmParser::classifySymbolRef(getImm())* /;
      return IsValid;
    } else {
      return isUInt<20>(Imm);
    }
  }

  bool isUImm20AUIPC() const {

    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    if (!IsConstantImm) {
      IsValid = false/ *EMBERAsmParser::classifySymbolRef(getImm())* /;
      return IsValid;
    } else {
      return isUInt<20>(Imm);
    }
  }

  bool isSImm21Lsb0JAL() const { return isBareSimmNLsb0<21>(); }
*/
  bool isImmZero() const {
    if (!isImm())
      return false;
    int64_t Imm;

    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && (Imm == 0);
  }

  bool isSImm5Plus1() const {
    if (!isImm())
      return false;

    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
    return IsConstantImm && isInt<5>(Imm - 1);

  }

    /// getStartLoc - Gets location of the first token of this operand
    SMLoc getStartLoc() const override { return StartLoc; }
    /// getEndLoc - Gets location of the last token of this operand
    SMLoc getEndLoc() const override { return EndLoc; }
    
    unsigned getReg() const override 
    {
        assert(Kind == KindTy::Register && "Invalid type access!");
        return Reg.RegNum.id();
    }
    
    unsigned getUserReg() const 
    {
      assert(Kind == KindTy::UserRegister && "Invalid type access!");
      return UserReg.RegNum.id();
    }
    
    const MCExpr *getImm() const 
    {
        assert(Kind == KindTy::Immediate && "Invalid type access!");
        return Imm.Val;
    }
    
    StringRef getToken() const
    {
        assert(Kind == KindTy::Token && "Invalid type access!");
        return Tok;
    }

    void print(raw_ostream &OS) const override 
    {
        auto RegName = [](unsigned Reg) 
        {
          if (Reg)
              return EMBERInstPrinter::getRegisterName(Reg);
          else
              return "noreg";
        };

        switch (Kind) 
        {
            case KindTy::Immediate:
                OS << *getImm();
                break;
            case KindTy::Register:
                OS << "<register " << RegName(getReg()) << ">";
                break;
            case KindTy::Token:
                OS << "'" << getToken() << "'";
                break;
            case KindTy::UserRegister:
                OS << "<sysreg: " << getUserReg() << '>';
                break;
//     case KindTy::VType:
//       OS << "<vtype: ";
// //       EMBERVType::printVType(getVType(), OS);
//       OS << '>';
//       break;
        }
    }

    static std::unique_ptr<EMBEROperand> createToken(StringRef Str, SMLoc S)
    {
        auto Op = std::make_unique<EMBEROperand>(KindTy::Token);
        Op->Tok = Str;
        Op->StartLoc = S;
        Op->EndLoc = S;
        return Op;
    }

    static std::unique_ptr<EMBEROperand> createReg(unsigned RegNo, SMLoc S, SMLoc E)
    {
        auto Op = std::make_unique<EMBEROperand>(KindTy::Register);
        Op->Reg.RegNum = RegNo;
        Op->StartLoc = S;
        Op->EndLoc = E;
        return Op;
    }

    static std::unique_ptr<EMBEROperand> createImm(const MCExpr *Val, SMLoc S, SMLoc E)
    {
        auto Op = std::make_unique<EMBEROperand>(KindTy::Immediate);
        Op->Imm.Val = Val;
        Op->StartLoc = S;
        Op->EndLoc = E;
        return Op;
    }

    static std::unique_ptr<EMBEROperand> createUserReg(unsigned RegNo, SMLoc S, SMLoc E)
    {
        auto Op = std::make_unique<EMBEROperand>(KindTy::UserRegister);
        Op->Reg.RegNum = RegNo;
        Op->StartLoc = S;
        Op->EndLoc = E;
        return Op;
    }

//   static std::unique_ptr<EMBEROperand> createVType(unsigned VTypeI, SMLoc S)
//   {
//     auto Op = std::make_unique<EMBEROperand>(KindTy::VType);
//     Op->VType.Val = VTypeI;
//     Op->StartLoc = S;
//     return Op;
//   }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const
  {
      assert(Expr && "Expr shouldn't be null!");
      int64_t Imm = 0;

      bool IsConstant = evaluateConstantImm(Expr, Imm);

      if (IsConstant)
          Inst.addOperand(MCOperand::createImm(Imm));
      else
          Inst.addOperand(MCOperand::createExpr(Expr));
  }

  // Used by the TableGen Code
  void addRegOperands(MCInst &Inst, unsigned N) const 
  {
      assert(N == 1 && "Invalid number of operands!");
      Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const
  {
      assert(N == 1 && "Invalid number of operands!");
      addExpr(Inst, getImm());
  }

  void addFenceArgOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    // isFenceArg has validated the operand, meaning this cast is safe
//     auto SE = cast<MCSymbolRefExpr>(getImm());

    unsigned Imm = 0;
//     for (char c : SE->getSymbol().getName()) {
//       switch (c) {
//       default:
//         llvm_unreachable("FenceArg must contain only [iorw]");
//       case 'i':
//         Imm |= EMBERFenceField::I;
//         break;
//       case 'o':
//         Imm |= EMBERFenceField::O;
//         break;
//       case 'r':
//         Imm |= EMBERFenceField::R;
//         break;
//       case 'w':
//         Imm |= EMBERFenceField::W;
//         break;
//       }
//     }
    Inst.addOperand(MCOperand::createImm(Imm));
  }

//   void addCSRSystemRegisterOperands(MCInst &Inst, unsigned N) const {
//     assert(N == 1 && "Invalid number of operands!");
//     Inst.addOperand(MCOperand::createImm(UserReg.Encoding));
//   }
// 
//   void addVTypeIOperands(MCInst &Inst, unsigned N) const {
//     assert(N == 1 && "Invalid number of operands!");
//     Inst.addOperand(MCOperand::createImm(getVType()));
//   }

  // Returns the rounding mode represented by this EMBEROperand. Should only
  // be called after checking isFRMArg.
/*
  EMBERFPRndMode::RoundingMode getRoundingMode() const {
    // isFRMArg has validated the operand, meaning this cast is safe.
    auto SE = cast<MCSymbolRefExpr>(getImm());
    EMBERFPRndMode::RoundingMode FRM =
        EMBERFPRndMode::stringToRoundingMode(SE->getSymbol().getName());
    assert(FRM != EMBERFPRndMode::Invalid && "Invalid rounding mode");
    return FRM;
  }

  void addFRMArgOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createImm(getRoundingMode()));
  }
*/
};
} // end anonymous namespace.

#define GET_REGISTER_MATCHER
#define GET_SUBTARGET_FEATURE_NAME
#define GET_MATCHER_IMPLEMENTATION
#define GET_MNEMONIC_SPELL_CHECKER
#include "EMBERGenAsmMatcher.inc"
/*

static MCRegister convertFPR64ToFPR16(MCRegister Reg) {
  assert(Reg >= EMBER::F0_D && Reg <= EMBER::F31_D && "Invalid register");
  return Reg - EMBER::F0_D + EMBER::F0_H;
}

static MCRegister convertFPR64ToFPR32(MCRegister Reg) {
  assert(Reg >= EMBER::F0_D && Reg <= EMBER::F31_D && "Invalid register");
  return Reg - EMBER::F0_D + EMBER::F0_F;
}

static MCRegister convertVRToVRMx(const MCRegisterInfo &RI, MCRegister Reg,
                                  unsigned Kind) {
  unsigned RegClassID;
  if (Kind == MCK_VRM2)
    RegClassID = EMBER::VRM2RegClassID;
  else if (Kind == MCK_VRM4)
    RegClassID = EMBER::VRM4RegClassID;
  else if (Kind == MCK_VRM8)
    RegClassID = EMBER::VRM8RegClassID;
  else
    return 0;
  return RI.getMatchingSuperReg(Reg, EMBER::sub_vrm1_0,
                                &EMBERMCRegisterClasses[RegClassID]);
}

unsigned EMBERAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                                    unsigned Kind) {
  EMBEROperand &Op = static_cast<EMBEROperand &>(AsmOp);
  if (!Op.isReg())
    return Match_InvalidOperand;

  MCRegister Reg = Op.getReg();
  bool IsRegFPR64 =
      EMBERMCRegisterClasses[EMBER::FPR64RegClassID].contains(Reg);
  bool IsRegFPR64C =
      EMBERMCRegisterClasses[EMBER::FPR64CRegClassID].contains(Reg);
  bool IsRegVR = EMBERMCRegisterClasses[EMBER::VRRegClassID].contains(Reg);

  // As the parser couldn't differentiate an FPR32 from an FPR64, coerce the
  // register from FPR64 to FPR32 or FPR64C to FPR32C if necessary.
  if ((IsRegFPR64 && Kind == MCK_FPR32) ||
      (IsRegFPR64C && Kind == MCK_FPR32C)) {
    Op.Reg.RegNum = convertFPR64ToFPR32(Reg);
    return Match_Success;
  }
  // As the parser couldn't differentiate an FPR16 from an FPR64, coerce the
  // register from FPR64 to FPR16 if necessary.
  if (IsRegFPR64 && Kind == MCK_FPR16) {
    Op.Reg.RegNum = convertFPR64ToFPR16(Reg);
    return Match_Success;
  }
  // As the parser couldn't differentiate an VRM2/VRM4/VRM8 from an VR, coerce
  // the register from VR to VRM2/VRM4/VRM8 if necessary.
  if (IsRegVR && (Kind == MCK_VRM2 || Kind == MCK_VRM4 || Kind == MCK_VRM8)) {
    Op.Reg.RegNum = convertVRToVRMx(*getContext().getRegisterInfo(), Reg, Kind);
    if (Op.Reg.RegNum == 0)
      return Match_InvalidOperand;
    return Match_Success;
  }
  return Match_InvalidOperand;
}
*/

bool EMBERAsmParser::generateImmOutOfRangeError(OperandVector &Operands, 
                                                uint64_t       ErrorInfo, 
                                                int64_t        Lower, 
                                                uint64_t       Upper,
                                                Twine Msg = "Immediate must be an integer in the range")
{
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, Msg + " [" + Twine(Lower) + ", " + Twine(Upper) + "]");
}

static std::string EMBERMnemonicSpellCheck(StringRef S, const FeatureBitset &FBS, unsigned VariantID = 0);

bool EMBERAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) 
{
    MCInst Inst;
    FeatureBitset MissingFeatures;

    auto Result = MatchInstructionImpl(Operands, Inst, ErrorInfo, MissingFeatures, MatchingInlineAsm);
    if (Result == Match_Success)
        Result = validateInstruction(Inst, ErrorInfo, Operands);

    switch (Result) 
    {
        default:
            break;
        case Match_Success:
        {
            return processInstruction(Inst, IDLoc, Operands, Out);
        }
        case Match_MissingFeature: 
        {
            assert(MissingFeatures.any() && "Unknown missing features!");
            bool FirstFeature = true;
            std::string Msg = "instruction requires the following:";
            for (unsigned i = 0, e = MissingFeatures.size(); i != e; ++i) 
            {
                if (MissingFeatures[i]) {
                Msg += FirstFeature ? " " : ", ";
                Msg += getSubtargetFeatureName(i);
                FirstFeature = false;
                }
            }
            return Error(IDLoc, Msg);
        }
        case Match_MnemonicFail: 
        {
            FeatureBitset FBS = ComputeAvailableFeatures(getSTI().getFeatureBits());
            std::string Suggestion = EMBERMnemonicSpellCheck(((EMBEROperand &)*Operands[0]).getToken(), FBS);
            return Error(IDLoc, "unrecognized instruction mnemonic" + Suggestion);
        }
        case Match_InvalidOperand: 
        {
            SMLoc ErrorLoc = IDLoc;
            if (ErrorInfo != ~0U)
            {
                if (ErrorInfo >= Operands.size())
                    return Error(ErrorLoc, "too few operands for instruction");

                ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
                if (ErrorLoc == SMLoc())
                    ErrorLoc = IDLoc;
            }
            return Error(ErrorLoc, "invalid operand for instruction");
        }
    }

    // Handle the case when the error message is of specific type
    // other than the generic Match_InvalidOperand, and the
    // corresponding operand is missing.
    if (Result > FIRST_TARGET_MATCH_RESULT_TY) 
    {
        SMLoc ErrorLoc = IDLoc;
        if (ErrorInfo != ~0U && ErrorInfo >= Operands.size())
            return Error(ErrorLoc, "too few operands for instruction");
    }

    switch (Result) 
    {
        default:
            break;
        case Match_InvalidSImm8:
            return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 7), (1 << 7) - 1);
        case Match_InvalidUImm8:
            return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 8) - 1);
        case Match_InvalidSImm14:
            return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 13), (1 << 13) - 1);
        case Match_InvalidUImm14:
            return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 14) - 1);
        case Match_InvalidSImm16:
            return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 15), (1 << 15) - 1);
        case Match_InvalidUImm16:
            return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 16) - 1);
        case Match_InvalidUImm32:
            return generateImmOutOfRangeError(Operands, ErrorInfo, 0, ((uint64_t)1 << 32) - 1, "Immediate or Label must resolve to a value in the range");
        case Match_InvalidBranchTarget:
            return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 21), (1 << 21) - 1, "Branch target must be a Register or Label. Constant literal offsets are not allowed.");
 /*
    case Match_InvalidImmZero: 
    {
        SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
        return Error(ErrorLoc, "immediate must be zero");
    }
    case Match_InvalidUImmLog2XLen:
        return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 5) - 1);
  case Match_InvalidUImmLog2XLenNonZero:
    if (isRV64())
      return generateImmOutOfRangeError(Operands, ErrorInfo, 1, (1 << 6) - 1);
    return generateImmOutOfRangeError(Operands, ErrorInfo, 1, (1 << 5) - 1);
  case Match_InvalidUImmLog2XLenHalf:
    if (isRV64())
      return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 5) - 1);
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 4) - 1);
  case Match_InvalidUImm5:
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 5) - 1);
  case Match_InvalidSImm5:
    return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 4),
                                      (1 << 4) - 1);
  case Match_InvalidSImm6:
    return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 5),
                                      (1 << 5) - 1);
  case Match_InvalidSImm6NonZero:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 5), (1 << 5) - 1,
        "immediate must be non-zero in the range");
  case Match_InvalidCLUIImm:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 1, (1 << 5) - 1,
        "immediate must be in [0xfffe0, 0xfffff] or");
  case Match_InvalidUImm7Lsb00:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 7) - 4,
        "immediate must be a multiple of 4 bytes in the range");
  case Match_InvalidUImm8Lsb00:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 8) - 4,
        "immediate must be a multiple of 4 bytes in the range");
  case Match_InvalidUImm8Lsb000:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 8) - 8,
        "immediate must be a multiple of 8 bytes in the range");
  case Match_InvalidSImm9Lsb0:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 8), (1 << 8) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidUImm9Lsb000:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 9) - 8,
        "immediate must be a multiple of 8 bytes in the range");
  case Match_InvalidUImm10Lsb00NonZero:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 4, (1 << 10) - 4,
        "immediate must be a multiple of 4 bytes in the range");
  case Match_InvalidSImm10Lsb0000NonZero:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 9), (1 << 9) - 16,
        "immediate must be a multiple of 16 bytes and non-zero in the range");
  case Match_InvalidSImm12:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 11), (1 << 11) - 1,
        "operand must be a symbol with %lo/%pcrel_lo/%tprel_lo modifier or an "
        "integer in the range");
  case Match_InvalidSImm12Lsb0:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 11), (1 << 11) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidSImm13Lsb0:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 12), (1 << 12) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidUImm20LUI:
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 20) - 1,
                                      "operand must be a symbol with "
                                      "%hi/%tprel_hi modifier or an integer in "
                                      "the range");
  case Match_InvalidUImm20AUIPC:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, 0, (1 << 20) - 1,
        "operand must be a symbol with a "
        "%pcrel_hi/%got_pcrel_hi/%tls_ie_pcrel_hi/%tls_gd_pcrel_hi modifier or "
        "an integer in the range");
  case Match_InvalidSImm21Lsb0JAL:
    return generateImmOutOfRangeError(
        Operands, ErrorInfo, -(1 << 20), (1 << 20) - 2,
        "immediate must be a multiple of 2 bytes in the range");
  case Match_InvalidCSRSystemRegister: {
    return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 12) - 1,
                                      "operand must be a valid system register "
                                      "name or an integer in the range");
  }
  case Match_InvalidFenceArg: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(
        ErrorLoc,
        "operand must be formed of letters selected in-order from 'iorw'");
  }
  case Match_InvalidFRMArg: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(
        ErrorLoc,
        "operand must be a valid floating point rounding mode mnemonic");
  }
  case Match_InvalidBareSymbol: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a bare symbol name");
  }
  case Match_InvalidPseudoJumpSymbol: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a valid jump target");
  }
  case Match_InvalidCallSymbol: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a bare symbol name");
  }
  case Match_InvalidTPRelAddSymbol: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be a symbol with %tprel_add modifier");
  }
  case Match_InvalidVTypeI: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(
        ErrorLoc,
        "operand must be "
        "e[8|16|32|64|128|256|512|1024],m[1|2|4|8|f2|f4|f8],[ta|tu],[ma|mu]");
  }
  case Match_InvalidVMaskRegister: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "operand must be v0.t");
  }
  case Match_InvalidSImm5Plus1: {
    return generateImmOutOfRangeError(Operands, ErrorInfo, -(1 << 4) + 1,
                                      (1 << 4),
                                      "immediate must be in the range");
  }
*/
    }

    llvm_unreachable("Unknown match type detected!");
}

bool EMBERAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                   SMLoc &EndLoc) {
  if (tryParseRegister(RegNo, StartLoc, EndLoc) != MatchOperand_Success)
    return Error(StartLoc, "invalid register name");
  return false;
}

OperandMatchResultTy EMBERAsmParser::tryParseRegister(unsigned &RegNo,
                                                      SMLoc &StartLoc,
                                                      SMLoc &EndLoc) {
  const AsmToken &Tok = getParser().getTok();
  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();
  StringRef Name = getLexer().getTok().getIdentifier();
  RegNo = MatchRegisterName(Name);

  if (RegNo == EMBER::NoRegister)
     return MatchOperand_NoMatch;

  getParser().Lex(); // Eat identifier token.
  return MatchOperand_Success;
}

OperandMatchResultTy EMBERAsmParser::parseRegister(OperandVector &Operands) 
{
    if (getLexer().getKind() == AsmToken::Identifier)
    {
        StringRef Name = getLexer().getTok().getIdentifier();
        MCRegister RegNo = MatchRegisterName(Name);
        if (RegNo == EMBER::NoRegister)
            return MatchOperand_NoMatch;

        SMLoc S = getLoc();
        SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
        getLexer().Lex();
        if (Name.front() == 'u')
            Operands.push_back(EMBEROperand::createUserReg(RegNo, S, E));
        else
            Operands.push_back(EMBEROperand::createReg(RegNo, S, E));
        return MatchOperand_Success;
    }

    return MatchOperand_NoMatch;
}
/*
OperandMatchResultTy
EMBERAsmParser::parseCSRSystemRegister(OperandVector &Operands) {
  SMLoc S = getLoc();
  const MCExpr *Res;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Exclaim:
  case AsmToken::Tilde:
  case AsmToken::Integer:
  case AsmToken::String: {
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;

    auto *CE = dyn_cast<MCConstantExpr>(Res);
    if (CE) {
      int64_t Imm = CE->getValue();
      if (isUInt<12>(Imm)) {
        auto SysReg = EMBERSysReg::lookupSysRegByEncoding(Imm);
        // Accept an immediate representing a named or un-named Sys Reg
        // if the range is valid, regardless of the required features.
        Operands.push_back(EMBEROperand::createUserReg(
            SysReg ? SysReg->Name : "", S, Imm, isRV64()));
        return MatchOperand_Success;
      }
    }

    Twine Msg = "immediate must be an integer in the range";
    Error(S, Msg + " [" + Twine(0) + ", " + Twine((1 << 12) - 1) + "]");
    return MatchOperand_ParseFail;
  }
  case AsmToken::Identifier: {
    StringRef Identifier;
    if (getParser().parseIdentifier(Identifier))
      return MatchOperand_ParseFail;

    auto SysReg = EMBERSysReg::lookupSysRegByName(Identifier);
    if (!SysReg)
      SysReg = EMBERSysReg::lookupSysRegByAltName(Identifier);
    // Accept a named Sys Reg if the required features are present.
    if (SysReg) {
      if (!SysReg->haveRequiredFeatures(getSTI().getFeatureBits())) {
        Error(S, "system register use requires an option to be enabled");
        return MatchOperand_ParseFail;
      }
      Operands.push_back(EMBEROperand::createUserReg(
          Identifier, S, SysReg->Encoding, isRV64()));
      return MatchOperand_Success;
    }

    Twine Msg = "operand must be a valid system register name "
                "or an integer in the range";
    Error(S, Msg + " [" + Twine(0) + ", " + Twine((1 << 12) - 1) + "]");
    return MatchOperand_ParseFail;
  }
  case AsmToken::Percent: {
    // Discard operand with modifier.
    Twine Msg = "immediate must be an integer in the range";
    Error(S, Msg + " [" + Twine(0) + ", " + Twine((1 << 12) - 1) + "]");
    return MatchOperand_ParseFail;
  }
  }

  return MatchOperand_NoMatch;
}
*/

OperandMatchResultTy EMBERAsmParser::parseBranchTarget(OperandVector &Operands)
{
    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    const MCExpr *Res;

    switch (getLexer().getKind())
    {
        default:
        case AsmToken::LParen:
        case AsmToken::Dot:
        case AsmToken::Exclaim:
        case AsmToken::Tilde:
        case AsmToken::String:
        case AsmToken::Percent:
            return MatchOperand_NoMatch;
        case AsmToken::Hash:
        {
            Lex();// need to remove hash if it's in front of a constant
            LLVM_FALLTHROUGH;
        }
        case AsmToken::Identifier:
        case AsmToken::Plus:
        case AsmToken::Minus:
        case AsmToken::Integer:
            if (getParser().parseExpression(Res))
                return MatchOperand_ParseFail;
            break;
    }

    Operands.push_back(EMBEROperand::createImm(Res, S, E));
    return MatchOperand_Success;
}

OperandMatchResultTy EMBERAsmParser::parseImmediate(OperandVector &Operands) 
{
    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    const MCExpr *Res;

    // TODO: remove some of these, percent, string, etc.
    switch (getLexer().getKind())
    {
        default:
        case AsmToken::Dot:
            return MatchOperand_NoMatch;
        case AsmToken::Hash:
        {
            Lex();// need to remove hash if it's in front of a constant
            LLVM_FALLTHROUGH;
//            [[clang::fallthrough]];
        }
        case AsmToken::Identifier:
        case AsmToken::LParen:
        case AsmToken::Minus:
        case AsmToken::Plus:
        case AsmToken::Exclaim:
        case AsmToken::Tilde:
        case AsmToken::Integer:
        case AsmToken::String:
        case AsmToken::Percent:
            if (getParser().parseExpression(Res))
                return MatchOperand_ParseFail;
            break;
    }

    Operands.push_back(EMBEROperand::createImm(Res, S, E));
    return MatchOperand_Success;
}

/*
OperandMatchResultTy
EMBERAsmParser::parseOperandWithModifier(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);

  if (getLexer().getKind() != AsmToken::Percent) {
    Error(getLoc(), "expected '%' for operand modifier");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat '%'

  if (getLexer().getKind() != AsmToken::Identifier) {
    Error(getLoc(), "expected valid identifier for operand modifier");
    return MatchOperand_ParseFail;
  }
  StringRef Identifier = getParser().getTok().getIdentifier();
  EMBERMCExpr::VariantKind VK = EMBERMCExpr::getVariantKindForName(Identifier);
  if (VK == EMBERMCExpr::VK_EMBER_Invalid) {
    Error(getLoc(), "unrecognized operand modifier");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat the identifier
  if (getLexer().getKind() != AsmToken::LParen) {
    Error(getLoc(), "expected '('");
    return MatchOperand_ParseFail;
  }
  getParser().Lex(); // Eat '('

  const MCExpr *SubExpr;
  if (getParser().parseParenExpression(SubExpr, E)) {
    return MatchOperand_ParseFail;
  }

  const MCExpr *ModExpr = EMBERMCExpr::create(SubExpr, VK, getContext());
  Operands.push_back(EMBEROperand::createImm(ModExpr, S, E, isRV64()));
  return MatchOperand_Success;
}
OperandMatchResultTy EMBERAsmParser::parseBareSymbol(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  if (getLexer().getKind() != AsmToken::Identifier)
    return MatchOperand_NoMatch;

  StringRef Identifier;
  AsmToken Tok = getLexer().getTok();

  if (getParser().parseIdentifier(Identifier))
    return MatchOperand_ParseFail;

  if (Identifier.consume_back("@plt")) {
    Error(getLoc(), "'@plt' operand not valid for instruction");
    return MatchOperand_ParseFail;
  }

  MCSymbol *Sym = getContext().getOrCreateSymbol(Identifier);

  if (Sym->isVariable()) {
    const MCExpr *V = Sym->getVariableValue(false);
    if (!isa<MCSymbolRefExpr>(V)) {
      getLexer().UnLex(Tok); // Put back if it's not a bare symbol.
      return MatchOperand_NoMatch;
    }
    Res = V;
  } else
    Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, getContext());

  MCBinaryExpr::Opcode Opcode;
  switch (getLexer().getKind()) {
  default:
    Operands.push_back(EMBEROperand::createImm(Res, S, E, isRV64()));
    return MatchOperand_Success;
  case AsmToken::Plus:
    Opcode = MCBinaryExpr::Add;
    break;
  case AsmToken::Minus:
    Opcode = MCBinaryExpr::Sub;
    break;
  }

  const MCExpr *Expr;
  if (getParser().parseExpression(Expr))
    return MatchOperand_ParseFail;
  Res = MCBinaryExpr::create(Opcode, Res, Expr, getContext());
  Operands.push_back(EMBEROperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy EMBERAsmParser::parseCallSymbol(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  if (getLexer().getKind() != AsmToken::Identifier)
    return MatchOperand_NoMatch;

  // Avoid parsing the register in `call rd, foo` as a call symbol.
  if (getLexer().peekTok().getKind() != AsmToken::EndOfStatement)
    return MatchOperand_NoMatch;

  StringRef Identifier;
  if (getParser().parseIdentifier(Identifier))
    return MatchOperand_ParseFail;

  EMBERMCExpr::VariantKind Kind = EMBERMCExpr::VK_EMBER_CALL;
  if (Identifier.consume_back("@plt"))
    Kind = EMBERMCExpr::VK_EMBER_CALL_PLT;

  MCSymbol *Sym = getContext().getOrCreateSymbol(Identifier);
  Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None, getContext());
  Res = EMBERMCExpr::create(Res, Kind, getContext());
  Operands.push_back(EMBEROperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy
EMBERAsmParser::parsePseudoJumpSymbol(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  if (getParser().parseExpression(Res))
    return MatchOperand_ParseFail;

  if (Res->getKind() != MCExpr::ExprKind::SymbolRef ||
      cast<MCSymbolRefExpr>(Res)->getKind() ==
          MCSymbolRefExpr::VariantKind::VK_PLT) {
    Error(S, "operand must be a valid jump target");
    return MatchOperand_ParseFail;
  }

  Res = EMBERMCExpr::create(Res, EMBERMCExpr::VK_EMBER_CALL, getContext());
  Operands.push_back(EMBEROperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

OperandMatchResultTy EMBERAsmParser::parseJALOffset(OperandVector &Operands) {
  // Parsing jal operands is fiddly due to the `jal foo` and `jal ra, foo`
  // both being acceptable forms. When parsing `jal ra, foo` this function
  // will be called for the `ra` register operand in an attempt to match the
  // single-operand alias. parseJALOffset must fail for this case. It would
  // seem logical to try parse the operand using parseImmediate and return
  // NoMatch if the next token is a comma (meaning we must be parsing a jal in
  // the second form rather than the first). We can't do this as there's no
  // way of rewinding the lexer state. Instead, return NoMatch if this operand
  // is an identifier and is followed by a comma.
  if (getLexer().is(AsmToken::Identifier);
      getLexer().peekTok().is(AsmToken::Comma))
    return MatchOperand_NoMatch;

  return parseImmediate(Operands);
}

OperandMatchResultTy EMBERAsmParser::parseVTypeI(OperandVector &Operands) {
  SMLoc S = getLoc();
  if (getLexer().isNot(AsmToken::Identifier))
    return MatchOperand_NoMatch;

  SmallVector<AsmToken, 7> VTypeIElements;
  // Put all the tokens for vtypei operand into VTypeIElements vector.
  while (getLexer().isNot(AsmToken::EndOfStatement)) {
    VTypeIElements.push_back(getLexer().getTok());
    getLexer().Lex();
    if (getLexer().is(AsmToken::EndOfStatement))
      break;
    if (getLexer().isNot(AsmToken::Comma))
      goto MatchFail;
    AsmToken Comma = getLexer().getTok();
    VTypeIElements.push_back(Comma);
    getLexer().Lex();
  }

  if (VTypeIElements.size() == 7) {
    // The VTypeIElements layout is:
    // SEW comma LMUL comma TA comma MA
    //  0    1    2     3    4   5    6
    StringRef Name = VTypeIElements[0].getIdentifier();
    if (!Name.consume_front("e"))
      goto MatchFail;
    unsigned Sew;
    if (Name.getAsInteger(10, Sew))
      goto MatchFail;
    if (!EMBERVType::isValidSEW(Sew))
      goto MatchFail;

    Name = VTypeIElements[2].getIdentifier();
    if (!Name.consume_front("m"))
      goto MatchFail;
    // "m" or "mf"
    bool Fractional = Name.consume_front("f");
    unsigned Lmul;
    if (Name.getAsInteger(10, Lmul))
      goto MatchFail;
    if (!EMBERVType::isValidLMUL(Lmul, Fractional))
      goto MatchFail;

    // ta or tu
    Name = VTypeIElements[4].getIdentifier();
    bool TailAgnostic;
    if (Name == "ta")
      TailAgnostic = true;
    else if (Name == "tu")
      TailAgnostic = false;
    else
      goto MatchFail;

    // ma or mu
    Name = VTypeIElements[6].getIdentifier();
    bool MaskAgnostic;
    if (Name == "ma")
      MaskAgnostic = true;
    else if (Name == "mu")
      MaskAgnostic = false;
    else
      goto MatchFail;

    unsigned SewLog2 = Log2_32(Sew / 8);
    unsigned LmulLog2 = Log2_32(Lmul);
    EMBERVSEW VSEW = static_cast<EMBERVSEW>(SewLog2);
    EMBERVLMUL VLMUL =
        static_cast<EMBERVLMUL>(Fractional ? 8 - LmulLog2 : LmulLog2);

    unsigned VTypeI =
        EMBERVType::encodeVTYPE(VLMUL, VSEW, TailAgnostic, MaskAgnostic);
    Operands.push_back(EMBEROperand::createVType(VTypeI, S, isRV64()));
    return MatchOperand_Success;
  }

// If NoMatch, unlex all the tokens that comprise a vtypei operand
MatchFail:
  while (!VTypeIElements.empty())
    getLexer().UnLex(VTypeIElements.pop_back_val());
  return MatchOperand_NoMatch;
}

OperandMatchResultTy EMBERAsmParser::parseMaskReg(OperandVector &Operands) {
  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::Identifier:
    StringRef Name = getLexer().getTok().getIdentifier();
    if (!Name.consume_back(".t")) {
      Error(getLoc(), "expected '.t' suffix");
      return MatchOperand_ParseFail;
    }
    MCRegister RegNo;
    matchRegisterNameHelper(isRV32E(), RegNo, Name);

    if (RegNo == EMBER::NoRegister)
      return MatchOperand_NoMatch;
    if (RegNo != EMBER::V0)
      return MatchOperand_NoMatch;
    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    getLexer().Lex();
    Operands.push_back(EMBEROperand::createReg(RegNo, S, E, isRV64()));
  }

  return MatchOperand_Success;
}
*/
OperandMatchResultTy EMBERAsmParser::parseMemOpBaseReg(OperandVector &Operands) 
{
    if (getLexer().isNot(AsmToken::LParen)) 
    {
        Error(getLoc(), "expected '('");
        return MatchOperand_ParseFail;
    }

    getParser().Lex(); // Eat '('
    Operands.push_back(EMBEROperand::createToken("(", getLoc()));

    if (parseRegister(Operands) != MatchOperand_Success)
    {
        Error(getLoc(), "expected register");
        return MatchOperand_ParseFail;
    }

    if (getLexer().is(AsmToken::Plus) || getLexer().is(AsmToken::Minus))
    {
        getParser().Lex(); // Eat '+ or -'

        // TODO: parse Imm offset inside address
        Error(getLoc(), "need to handle imm offset in address");

        return MatchOperand_ParseFail;
    }

    if (getLexer().isNot(AsmToken::RParen)) 
    {
        Error(getLoc(), "expected ')'");
        return MatchOperand_ParseFail;
    }

    getParser().Lex(); // Eat ')'
    Operands.push_back(EMBEROperand::createToken(")", getLoc()));

    return MatchOperand_Success;
}
/*
OperandMatchResultTy EMBERAsmParser::parseAtomicMemOp(OperandVector &Operands) {
  // Atomic operations such as lr.w, sc.w, and amo*.w accept a "memory operand"
  // as one of their register operands, such as `(a0)`. This just denotes that
  // the register (in this case `a0`) contains a memory address.
  //
  // Normally, we would be able to parse these by putting the parens into the
  // instruction string. However, GNU as also accepts a zero-offset memory
  // operand (such as `0(a0)`), and ignores the 0. Normally this would be parsed
  // with parseImmediate followed by parseMemOpBaseReg, but these instructions
  // do not accept an immediate operand, and we do not want to add a "dummy"
  // operand that is silently dropped.
  //
  // Instead, we use this custom parser. This will: allow (and discard) an
  // offset if it is zero; require (and discard) parentheses; and add only the
  // parsed register operand to `Operands`.
  //
  // These operands are printed with EMBERInstPrinter::printAtomicMemOp, which
  // will only print the register surrounded by parentheses (which GNU as also
  // uses as its canonical representation for these operands).
  std::unique_ptr<EMBEROperand> OptionalImmOp;

  if (getLexer().isNot(AsmToken::LParen)) {
    // Parse an Integer token. We do not accept arbritrary constant expressions
    // in the offset field (because they may include parens, which complicates
    // parsing a lot).
    int64_t ImmVal;
    SMLoc ImmStart = getLoc();
    if (getParser().parseIntToken(ImmVal,
                                  "expected '(' or optional integer offset"))
      return MatchOperand_ParseFail;

    // Create a EMBEROperand for checking later (so the error messages are
    // nicer), but we don't add it to Operands.
    SMLoc ImmEnd = getLoc();
    OptionalImmOp =
        EMBEROperand::createImm(MCConstantExpr::create(ImmVal, getContext()),
                                ImmStart, ImmEnd, isRV64());
  }

  if (getLexer().isNot(AsmToken::LParen)) {
    Error(getLoc(), OptionalImmOp ? "expected '(' after optional integer offset"
                                  : "expected '(' or optional integer offset");
    return MatchOperand_ParseFail;
  }
  getParser().Lex(); // Eat '('

  if (parseRegister(Operands) != MatchOperand_Success) {
    Error(getLoc(), "expected register");
    return MatchOperand_ParseFail;
  }

  if (getLexer().isNot(AsmToken::RParen)) {
    Error(getLoc(), "expected ')'");
    return MatchOperand_ParseFail;
  }
  getParser().Lex(); // Eat ')'

  // Deferred Handling of non-zero offsets. This makes the error messages nicer.
  if (OptionalImmOp && !OptionalImmOp->isImmZero()) {
    Error(OptionalImmOp->getStartLoc(), "optional integer offset must be 0",
          SMRange(OptionalImmOp->getStartLoc(), OptionalImmOp->getEndLoc()));
    return MatchOperand_ParseFail;
  }

  return MatchOperand_Success;
}
*/

/// Looks at a token type and creates the relevant operand from this
/// information, adding to Operands. If operand was parsed, returns false, else
/// true.
bool EMBERAsmParser::parseOperand(OperandVector &Operands, 
                                  StringRef      Mnemonic) 
{

    // Attempt to parse token as a register.
    if (parseRegister(Operands) == MatchOperand_Success)
        return false;

    // auto parse imm/symbol values
    if (MatchOperandParserImpl(Operands, Mnemonic, /*ParseForAllFeatures=*/true) == MatchOperand_Success)
        return false;

//     // Attempt to parse token as a branch target reference.
//     if (parseBranchTarget(Operands) == MatchOperand_Success)
//         return false;

    // Attempt to parse token as an immediate
    if (parseImmediate(Operands) == MatchOperand_Success) 
        return false;

    // Attempt to parse as address
    if (getLexer().is(AsmToken::LParen))
        return parseMemOpBaseReg(Operands) != MatchOperand_Success;
    
    // Finally we have exhausted all options and must declare defeat.
    Error(getLoc(), "unknown operand");
    return true;
}

bool EMBERAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                      StringRef             Name, 
                                      SMLoc                 NameLoc,
                                      OperandVector        &Operands)
{
    // First operand is token for instruction
    Operands.push_back(EMBEROperand::createToken(Name, NameLoc));

    // If there are no more operands, then finish
    if (getLexer().is(AsmToken::EndOfStatement))
        return false;

    // Parse first operand
    if (parseOperand(Operands, Name))
        return true;

    // TODO: ( for st* or ld*? Maybe done in ParseOp?

    // Parse until end of statement, consuming commas between operands
    unsigned OperandIdx = 1;
    while (getLexer().is(AsmToken::Comma))
    {
        // Consume comma token
        getLexer().Lex();

        // Parse next operand
        if (parseOperand(Operands, Name))
            return true;

        ++OperandIdx;
    }

    if (getLexer().isNot(AsmToken::EndOfStatement)) 
    {
        SMLoc Loc = getLexer().getLoc();
        getParser().eatToEndOfStatement();
        return Error(Loc, "unexpected token");
    }

    getParser().Lex(); // Consume the EndOfStatement.
    return false;
}

bool EMBERAsmParser::ParseDirective(AsmToken DirectiveID) 
{
    // This returns false if this function recognizes the directive
    // regardless of whether it is successfully handles or reports an
    // error. Otherwise it returns true to give the generic parser a
    // chance at recognizing it.
    StringRef IDVal = DirectiveID.getString();

    if (IDVal == ".set_addr")
    {
        MCAsmParser& Parser = getParser();

        // Get the address value token.
        AsmToken Tok = Parser.getTok();

        // At the moment only constant value addresses are supported.
        if (Tok.isNot(AsmToken::Integer))
            return Error(Parser.getTok().getLoc(), "unexpected token, expected identifier");

        const MCExpr* AttrExpr;
        int64_t Tag;
        SMLoc TagLoc;

        TagLoc = Parser.getTok().getLoc();
        if (Parser.parseExpression(AttrExpr))
            return true;

        const MCConstantExpr* CE = dyn_cast<MCConstantExpr>(AttrExpr);
        if (check(!CE, TagLoc, "expected numeric constant"))
            return true;

        Tag = CE->getValue();

        // TODO: Where to store this address so it can be added in to the values in EMBERAsmBackend::evaluateTargetFixup()


//         MCAssembler& MCA = getAssembler();
//         auto& MAB = static_cast<EMBERAsmBackend&>(MCA.getBackend());


    }

    return true;
}

void EMBERAsmParser::emitToStreamer(MCStreamer &S, const MCInst &Inst)
{
    MCInst CInst;

    S.emitInstruction(Inst, getSTI());
}

void EMBERAsmParser::emitLDIImm(MCInst& Inst, SMLoc IDLoc, MCStreamer& Out)
{
    // .w  - zero-extend 
    // .h  - zero-extend 
    // .sh - sign-extend

    // .b  - zero-extend 
    // .sh - sign-extend


    // If Imm fits in 16 bits, emit:
    //      ldi rd, $xxxx
    // 
    // Otherwise, need ldi+ldih
    //      ldi  rd, $llll
    //      ldih rd, $hhhh
    // 
    MCOperand DestReg = Inst.getOperand(0);
    MCOperand SourceImm = Inst.getOperand(1);
    if (SourceImm.isImm())
    {
        // Test only cases where the value CAN be larger than 16-bits
        uint64_t value = SourceImm.getImm();
        switch (Inst.getOpcode()) 
        {
            case EMBER::LDI_w_lo:
            case EMBER::LDI_hh_lo:
            case EMBER::LDI_bbbb_lo: //?
                if (static_cast<uint16_t>(value) != value)
                    break;
            default: 
                emitToStreamer(Out, Inst);
                return; 
        }
    }

    // >16-bit values are valid and we have one, need to add an additional LDIH instruction
    unsigned LDIHOpcode = 0;
    switch (Inst.getOpcode()) 
    {
        case EMBER::LDI_w_lo:
            LDIHOpcode = EMBER::LDI_w_hi;
            break;
        case EMBER::LDI_hh_lo:
            LDIHOpcode = EMBER::LDI_hh_hi;
            break;
        case EMBER::LDI_bbbb_lo:
            LDIHOpcode = EMBER::LDI_bbbb_hi;
            break;
        default:
            Error(IDLoc, "only 32-bit types can be extended with LDIH");
    }

    emitToStreamer(Out, Inst);

    // If it is a constant immediate value, shift it properly (if not, it's a fixup and that will be done later)
    if (SourceImm.isImm())
        SourceImm.setImm(SourceImm.getImm()>>16);

    emitToStreamer(Out, MCInstBuilder(LDIHOpcode)
        .addOperand(DestReg)
        .addOperand(SourceImm));
}


unsigned  EMBERAsmParser::validateInstruction(MCInst &Inst, uint64_t &ErrorInfo, OperandVector &Operands) 
{
    // Any special validations?
    switch (Inst.getOpcode())
    {
        default:
            break;
        // For some LDI opcode widths, determine if immediate value fits in N bits
        case EMBER::LDI_h_lo:
        case EMBER::LDI_bb_lo: 
        {
            MCOperand SourceImm = Inst.getOperand(1);
            if (SourceImm.isImm())
            {
                uint64_t value = SourceImm.getImm();
                if (static_cast<uint16_t>(value) == value)
                    break;
            }
            ErrorInfo = 2; // rd==0, rA==1, rB/Imm==2? should be 1 for LDI
            return Match_InvalidUImm16;
        }
        case EMBER::LDI_sh_lo:
        {
            MCOperand SourceImm = Inst.getOperand(1);
            if (SourceImm.isImm())
            {
                uint64_t value = SourceImm.getImm();
                if (static_cast<uint16_t>(value) == value)
                    break;
            }
            ErrorInfo = 2;
            return Match_InvalidSImm16;
        }
        case EMBER::LDI_b_lo:
        {
            MCOperand SourceImm = Inst.getOperand(1);
            if (SourceImm.isImm())
            {
                uint64_t value = SourceImm.getImm();
                if (static_cast<uint8_t>(value) == value)
                    break;
            }
            ErrorInfo = 2;
            return Match_InvalidUImm8;
        }
        case EMBER::LDI_sb_lo:
        {
            MCOperand SourceImm = Inst.getOperand(1);
            if (SourceImm.isImm())
            {
                uint64_t value = SourceImm.getImm();
                if (static_cast<uint8_t>(value) == value)
                    break;
            }
            ErrorInfo = 2;
            return Match_InvalidSImm8;
        }
    }

  return Match_Success;
}


bool EMBERAsmParser::processInstruction(MCInst         &Inst, 
                                        SMLoc           IDLoc,
                                        OperandVector  &Operands,
                                        MCStreamer     &Out) 
{
    Inst.setLoc(IDLoc);

    switch (Inst.getOpcode())
    {
        // TODO: Check based on the type, if the imm value will fit, if we need to gen an LDIH, or if an error
    default:
        break;
        // For LDI opcode, determine if immediate value fits in 16 bits, or add an additional LDIH
    case EMBER::LDI_w_lo:
    case EMBER::LDI_h_lo:
    case EMBER::LDI_b_lo:
    case EMBER::LDI_sh_lo:
    case EMBER::LDI_sb_lo:
    case EMBER::LDI_hh_lo:
    case EMBER::LDI_bb_lo:
    case EMBER::LDI_bbbb_lo:
        emitLDIImm(Inst, IDLoc, Out);
        return false;
    }

    emitToStreamer(Out, Inst);
    return false;
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEMBERAsmParser() 
{
    RegisterMCAsmParser<EMBERAsmParser> X(getTheEMBER32Target());
}
