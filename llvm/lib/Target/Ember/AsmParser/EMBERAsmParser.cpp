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


class EMBERAsmParser : public MCTargetAsmParser
{
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
//         VType,
    } Kind;

    struct RegOp 
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
//             case KindTy::VType:
//               VType = o.VType;
//               break;
        }
    }

  bool isToken() const override { return Kind == KindTy::Token; }
  bool isReg() const override { return Kind == KindTy::Register; }
  bool isImm() const override { return Kind == KindTy::Immediate; }
  bool isMem() const override { return false; }
//   bool isVType() const { return Kind == KindTy::VType; }

  bool isGPRAsmReg() const { return Kind == KindTy::Register && Reg.RegNum < EMBER::PC; }
  bool isSYSAsmReg() const { return Kind == KindTy::Register && Reg.RegNum >= EMBER::PC; }

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


  bool isBranchTarget() const
  {
      int64_t Imm;

      // Must be of 'immediate' type but not a constant (due to relaxation, a constant offset is dangerous, so we just don't allow them).
      if (!isImm() || evaluateConstantImm(getImm(), Imm))
        return false;

      return true;
  }


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

        // otherwise, it's a label or whatever, and will need a fixup anyway, so just return true for now
        return true;
    }

//   bool isImmZero() const {
//     if (!isImm())
//       return false;
//     int64_t Imm;
// 
//     bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
//     return IsConstantImm && (Imm == 0);
//   }
// 
//   bool isSImm5Plus1() const {
//     if (!isImm())
//       return false;
// 
//     int64_t Imm;
//     bool IsConstantImm = evaluateConstantImm(getImm(), Imm);
//     return IsConstantImm && isInt<5>(Imm - 1);
// 
//   }

    /// getStartLoc - Gets location of the first token of this operand
    SMLoc getStartLoc() const override { return StartLoc; }
    /// getEndLoc - Gets location of the last token of this operand
    SMLoc getEndLoc() const override { return EndLoc; }
    
    unsigned getReg() const override 
    {
        assert(Kind == KindTy::Register && "Invalid type access!");
        return Reg.RegNum.id();
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

};
} // end anonymous namespace.

#define GET_REGISTER_MATCHER
#define GET_SUBTARGET_FEATURE_NAME
#define GET_MATCHER_IMPLEMENTATION
#define GET_MNEMONIC_SPELL_CHECKER
#include "EMBERGenAsmMatcher.inc"

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
        if (RegNo != EMBER::NoRegister) 
        {
            SMLoc S = getLoc();
            SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
            getLexer().Lex();
            Operands.push_back(EMBEROperand::createReg(RegNo, S, E));
            return MatchOperand_Success;
        }
    }

    return MatchOperand_NoMatch;
}

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

OperandMatchResultTy EMBERAsmParser::parseMemOpBaseReg(OperandVector &Operands) 
{
    if (getLexer().isNot(AsmToken::LParen)) 
    {
        Error(getLoc(), "expected '('");
        return MatchOperand_ParseFail;
    }

    getParser().Lex(); // Eat '('

    if (parseRegister(Operands) != MatchOperand_Success)
    {
        Error(getLoc(), "expected register");
        return MatchOperand_ParseFail;
    }

    uint16_t multiplier = getLexer().is(AsmToken::Plus) ? 1 : getLexer().is(AsmToken::Minus) ? -1 : 0; 
    if (multiplier != 0)
    {
//        getParser().Lex(); // Eat '+ or -'

        // Parse Imm offset inside address
        const MCExpr *OffsetExp;
        if (getParser().parseExpression(OffsetExp))
        {
            Error(getLoc(), "Expected immediate offset in address");
            return MatchOperand_ParseFail;
        }
        
//         int64_t Imm;
//         bool IsConstantImm = EMBEROperand::evaluateConstantImm(OffsetExp, Imm);

        SMLoc S = getLoc();
        SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
        Operands.push_back(EMBEROperand::createImm(OffsetExp, S, E));
//         Operands.push_back(&MCOperand::createImm(int16_t(Imm * multiplier)));
// 
//             emitInstWithOffset(MCOperand::createImm(int16_t(LoOffset)));
// 
// 
//         Operands.back()
    }

    if (getLexer().isNot(AsmToken::RParen)) 
    {
        Error(getLoc(), "expected ')'");
        return MatchOperand_ParseFail;
    }

    getParser().Lex(); // Eat ')'

    return MatchOperand_Success;
}

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

    // Attempt to parse as address
    if (getLexer().is(AsmToken::LParen)) 
    {
        if (parseMemOpBaseReg(Operands) == MatchOperand_Success)
            return false;
    }
    
    // Attempt to parse token as an immediate
    if (parseImmediate(Operands) == MatchOperand_Success) 
        return false;

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
