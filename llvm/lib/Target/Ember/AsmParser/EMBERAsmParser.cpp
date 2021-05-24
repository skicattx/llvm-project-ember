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
//#include "llvm/Support/EMBERAttributes.h"
#include "llvm/Support/TargetRegistry.h"

#include <limits>

using namespace llvm;

#define DEBUG_TYPE "ember-asm-parser"

// Include the auto-generated portion of the compress emitter.
// #define GEN_COMPRESS_INSTR
// #include "EMBERGenCompressInstEmitter.inc"

// STATISTIC(EMBERNumInstrsCompressed,
//           "Number of Ember Compressed instructions emitted");
namespace {
struct EMBEROperand;

struct ParserOptionsSet 
{
  bool IsPicEnabled;
};

class EMBERAsmParser : public MCTargetAsmParser
{
  SmallVector<FeatureBitset, 4> FeatureBitStack;

  SmallVector<ParserOptionsSet, 4> ParserOptionsStack;
  ParserOptionsSet ParserOptions;

  SMLoc getLoc() const { return getParser().getTok().getLoc(); }
/*
//   bool isRV64() const { return getSTI().hasFeature(EMBER::Feature64Bit); }
//   bool isRV32E() const { return getSTI().hasFeature(EMBER::FeatureRV32E); }

  EMBERTargetStreamer &getTargetStreamer() {
    MCTargetStreamer &TS = *getParser().getStreamer().getTargetStreamer();
    return static_cast<EMBERTargetStreamer &>(TS);
  }

  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op,
                                      unsigned Kind) override;

  bool generateImmOutOfRangeError(OperandVector &Operands, uint64_t ErrorInfo,
                                  int64_t Lower, int64_t Upper, Twine Msg);

*/
//  bool parsePrimaryExpr(const MCExpr*& Res, SMLoc& EndLoc, AsmTypeInfo* TypeInfo) override;

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

/*
  // Helper to actually emit an instruction to the MCStreamer. Also, when
  // possible, compression of the instruction is performed.
  void emitToStreamer(MCStreamer &S, const MCInst &Inst);

  // Helper to emit a combination of LUI, ADDI(W), and SLLI instructions that
  // synthesize the desired immedate value into the destination register.
  void emitLoadImm(MCRegister DestReg, int64_t Value, MCStreamer &Out);

  // Helper to emit a combination of AUIPC and SecondOpcode. Used to implement
  // helpers such as emitLoadLocalAddress and emitLoadAddress.
  void emitAuipcInstPair(MCOperand DestReg, MCOperand TmpReg,
                         const MCExpr *Symbol, EMBERMCExpr::VariantKind VKHi,
                         unsigned SecondOpcode, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "lla" used in PC-rel addressing.
  void emitLoadLocalAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "la" used in GOT/PC-rel addressing.
  void emitLoadAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "la.tls.ie" used in initial-exec TLS
  // addressing.
  void emitLoadTLSIEAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo instruction "la.tls.gd" used in global-dynamic TLS
  // addressing.
  void emitLoadTLSGDAddress(MCInst &Inst, SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo load/store instruction with a symbol.
  void emitLoadStoreSymbol(MCInst &Inst, unsigned Opcode, SMLoc IDLoc,
                           MCStreamer &Out, bool HasTmpReg);

  // Helper to emit pseudo sign/zero extend instruction.
  void emitPseudoExtend(MCInst &Inst, bool SignExtend, int64_t Width,
                        SMLoc IDLoc, MCStreamer &Out);

  // Helper to emit pseudo vmsge{u}.vx instruction.
  void emitVMSGE(MCInst &Inst, unsigned Opcode, SMLoc IDLoc, MCStreamer &Out);

  // Checks that a PseudoAddTPRel is using x4/tp in its second input operand.
  // Enforcing this using a restricted register class for the second input
  // operand of PseudoAddTPRel results in a poor diagnostic due to the fact
  // 'add' is an overloaded mnemonic.
  bool checkPseudoAddTPRel(MCInst &Inst, OperandVector &Operands);

  // Check instruction constraints.
  bool validateInstruction(MCInst &Inst, OperandVector &Operands);

  /// Helper for processing MC instructions that have been successfully matched
  /// by MatchAndEmitInstruction. Modifications to the emitted instructions,
  /// like the expansion of pseudo instructions (e.g., "li"), can be performed
  /// in this method.
  bool processInstruction(MCInst &Inst, SMLoc IDLoc, OperandVector &Operands,
                          MCStreamer &Out);
*/
// Auto-generated instruction matching functions
#define GET_ASSEMBLER_HEADER
#include "EMBERGenAsmMatcher.inc"
/*
  OperandMatchResultTy parseCSRSystemRegister(OperandVector &Operands);
  OperandMatchResultTy parseImmediate(OperandVector &Operands);
  OperandMatchResultTy parseRegister(OperandVector &Operands,
                                     bool AllowParens = false);
  OperandMatchResultTy parseMemOpBaseReg(OperandVector &Operands);
  OperandMatchResultTy parseAtomicMemOp(OperandVector &Operands);
  OperandMatchResultTy parseOperandWithModifier(OperandVector &Operands);
  OperandMatchResultTy parseBareSymbol(OperandVector &Operands);
  OperandMatchResultTy parseCallSymbol(OperandVector &Operands);
  OperandMatchResultTy parsePseudoJumpSymbol(OperandVector &Operands);
  OperandMatchResultTy parseJALOffset(OperandVector &Operands);
  OperandMatchResultTy parseVTypeI(OperandVector &Operands);
  OperandMatchResultTy parseMaskReg(OperandVector &Operands);
  */
  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);
  /*
  bool parseDirectiveOption();
  bool parseDirectiveAttribute();

  void setFeatureBits(uint64_t Feature, StringRef FeatureString) {
    if (!(getSTI().getFeatureBits()[Feature])) {
      MCSubtargetInfo &STI = copySTI();
      setAvailableFeatures(
          ComputeAvailableFeatures(STI.ToggleFeature(FeatureString)));
    }
  }

  bool getFeatureBits(uint64_t Feature) {
    return getSTI().getFeatureBits()[Feature];
  }

  void clearFeatureBits(uint64_t Feature, StringRef FeatureString) {
    if (getSTI().getFeatureBits()[Feature]) {
      MCSubtargetInfo &STI = copySTI();
      setAvailableFeatures(
          ComputeAvailableFeatures(STI.ToggleFeature(FeatureString)));
    }
  }

  void pushFeatureBits() {
    assert(FeatureBitStack.size() == ParserOptionsStack.size() &&
           "These two stacks must be kept synchronized");
    FeatureBitStack.push_back(getSTI().getFeatureBits());
    ParserOptionsStack.push_back(ParserOptions);
  }

  bool popFeatureBits() {
    assert(FeatureBitStack.size() == ParserOptionsStack.size() &&
           "These two stacks must be kept synchronized");
    if (FeatureBitStack.empty())
      return true;

    FeatureBitset FeatureBits = FeatureBitStack.pop_back_val();
    copySTI().setFeatureBits(FeatureBits);
    setAvailableFeatures(ComputeAvailableFeatures(FeatureBits));

    ParserOptions = ParserOptionsStack.pop_back_val();

    return false;
  }

  std::unique_ptr<EMBEROperand> defaultMaskRegOp() const;
*/
public:
  enum EMBERMatchResultTy {
    Match_Dummy = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "EMBERGenAsmMatcher.inc"
#undef GET_OPERAND_DIAGNOSTIC_TYPES
  };

/*
  static bool classifySymbolRef(const MCExpr *Expr,
                                EMBERMCExpr::VariantKind &Kind);
*/
    EMBERAsmParser(const MCSubtargetInfo &STI, MCAsmParser &Parser, const MCInstrInfo &MII, const MCTargetOptions &Options) : 
        MCTargetAsmParser(Options, STI, MII) 
    {
        Parser.addAliasForDirective(".half", ".2byte");
        Parser.addAliasForDirective(".hword", ".2byte");
        Parser.addAliasForDirective(".word", ".4byte");
        Parser.addAliasForDirective(".dword", ".8byte");

        // No feature currently, add in td file when needed
        setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));

//     auto ABIName = StringRef(Options.ABIName);
//     if (ABIName.endswith("f") &&
//         !getSTI().getFeatureBits()[EMBER::FeatureStdExtF]) {
//       errs() << "Hard-float 'f' ABI can't be used for a target that "
//                 "doesn't support the F instruction set extension (ignoring "
//                 "target-abi)\n";
//     } else if (ABIName.endswith("d") &&
//                !getSTI().getFeatureBits()[EMBER::FeatureStdExtD]) {
//       errs() << "Hard-float 'd' ABI can't be used for a target that "
//                 "doesn't support the D instruction set extension (ignoring "
//                 "target-abi)\n";
//     }

        const MCObjectFileInfo *MOFI = Parser.getContext().getObjectFileInfo();
        ParserOptions.IsPicEnabled = MOFI->isPositionIndependent();
    }
};

/// EMBEROperand - Instances of this class represent a parsed machine instruction
struct EMBEROperand : public MCParsedAsmOperand {

  enum class KindTy {
    Token,
    Register,
    Immediate,
    UserRegister,
    VType,
  } Kind;

  struct RegOp {
    MCRegister RegNum;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  struct SysRegOp {
    const char *Data;
    unsigned Length;
    unsigned Encoding;
    // FIXME: Add the Encoding parsed fields as needed for checks,
    // e.g.: read/write or user/supervisor/machine privileges.
  };

  struct VTypeOp {
    unsigned Val;
  };

  SMLoc StartLoc, EndLoc;
  union {
    StringRef Tok;
    RegOp Reg;
    ImmOp Imm;
    struct SysRegOp SysReg;
    struct VTypeOp VType;
  };

  EMBEROperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

public:
  EMBEROperand(const EMBEROperand &o) : MCParsedAsmOperand() {
    Kind = o.Kind;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch (Kind) {
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
      SysReg = o.SysReg;
      break;
    case KindTy::VType:
      VType = o.VType;
      break;
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
  bool isVType() const { return Kind == KindTy::VType; }

  bool isSImm14() const  { return Kind == KindTy::Immediate; /*TODO: one of these for each Imm bit count?*/ }
  bool isGPRAsmReg() const { return Kind == KindTy::Register; /*TODO: is this one of the u registers, or sp?*/}
  bool isSYSAsmReg() const { return Kind == KindTy::Register; /*TODO: is this one of the u registers, or sp?*/}
  bool isSPAsmReg() const { return Kind == KindTy::Register; /*TODO: is this one of the u registers, or sp?*/}

  static bool evaluateConstantImm(const MCExpr *Expr, int64_t &Imm, EMBERMCExpr::VariantKind &VK) {
    if (auto *RE = dyn_cast<EMBERMCExpr>(Expr)) 
    {
      VK = EMBERMCExpr::VK_EMBER_None; // RE->getKind();
      return RE->evaluateAsConstant(Imm);
    }

    if (auto CE = dyn_cast<MCConstantExpr>(Expr)) 
    {
      VK = EMBERMCExpr::VK_EMBER_None;
      Imm = CE->getValue();
      return true;
    }

    return false;
  }

  // True if operand is a symbol with no modifiers, or a constant with no
  // modifiers and isShiftedInt<N-1, 1>(Op).
  template <int N> bool isBareSimmNLsb0() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    bool IsValid;
    if (!IsConstantImm)
      IsValid = false/*EMBERAsmParser::classifySymbolRef(getImm(), VK)*/;
    else
      IsValid = isShiftedInt<N - 1, 1>(Imm);
    return IsValid && VK == EMBERMCExpr::VK_EMBER_None;
  }

  // Predicate methods for AsmOperands defined in EMBERInstrInfo.td

  bool isBareSymbol() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return /*EMBERAsmParser::classifySymbolRef(getImm(), VK) &&*/
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isCallSymbol() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return /*EMBERAsmParser::classifySymbolRef(getImm(), VK) &&*/
           (VK == EMBERMCExpr::VK_EMBER_CALL ||
            VK == EMBERMCExpr::VK_EMBER_CALL_PLT);
  }

  bool isPseudoJumpSymbol() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return /*EMBERAsmParser::classifySymbolRef(getImm(), VK) &&*/
           VK == EMBERMCExpr::VK_EMBER_CALL;
  }

  bool isTPRelAddSymbol() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    // Must be of 'immediate' type but not a constant.
    if (!isImm() || evaluateConstantImm(getImm(), Imm, VK))
      return false;
    return /*EMBERAsmParser::classifySymbolRef(getImm(), VK) &&*/
           VK == EMBERMCExpr::VK_EMBER_TPREL_ADD;
  }

  bool isCSRSystemRegister() const { return isSystemRegister(); }

  bool isVTypeI() const { return isVType(); }

  /// Return true if the operand is a valid for the fence instruction e.g.
  /// ('iorw').
  bool isFenceArg() const {
    if (!isImm())
      return false;
    const MCExpr *Val = getImm();
    auto *SVal = dyn_cast<MCSymbolRefExpr>(Val);
    if (!SVal || SVal->getKind() != MCSymbolRefExpr::VK_None)
      return false;

    StringRef Str = SVal->getSymbol().getName();
    // Letters must be unique, taken from 'iorw', and in ascending order. This
    // holds as long as each individual character is one of 'iorw' and is
    // greater than the previous character.
    char Prev = '\0';
    for (char c : Str) {
      if (c != 'i' && c != 'o' && c != 'r' && c != 'w')
        return false;
      if (c <= Prev)
        return false;
      Prev = c;
    }
    return true;
  }

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
*/

  bool isImmXLenLI() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (VK == EMBERMCExpr::VK_EMBER_LO || VK == EMBERMCExpr::VK_EMBER_PCREL_LO)
      return true;
    // Given only Imm, ensuring that the actually specified constant is either
    // a signed or unsigned 64-bit number is unfortunately impossible.
    return IsConstantImm && VK == EMBERMCExpr::VK_EMBER_None &&
           ((isInt<32>(Imm) || isUInt<32>(Imm)));
  }

  bool isUImmLog2XLen() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm, VK) ||
        VK != EMBERMCExpr::VK_EMBER_None)
      return false;
    return (isUInt<6>(Imm)) || isUInt<5>(Imm);
  }

  bool isUImmLog2XLenNonZero() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm, VK) ||
        VK != EMBERMCExpr::VK_EMBER_None)
      return false;
    if (Imm == 0)
      return false;
    return (isUInt<6>(Imm)) || isUInt<5>(Imm);
  }

  bool isUImmLog2XLenHalf() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    if (!isImm())
      return false;
    if (!evaluateConstantImm(getImm(), Imm, VK) ||
        VK != EMBERMCExpr::VK_EMBER_None)
      return false;
    return (isUInt<5>(Imm)) || isUInt<4>(Imm);
  }

  bool isUImm5() const {
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isUInt<5>(Imm) && VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isSImm5() const {
    if (!isImm())
      return false;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<5>(Imm) && VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isSImm6() const {
    if (!isImm())
      return false;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<6>(Imm) && VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isSImm6NonZero() const {
    if (!isImm())
      return false;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<6>(Imm) && (Imm != 0) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isCLUIImm() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && (Imm != 0) &&
           (isUInt<5>(Imm) || (Imm >= 0xfffe0 && Imm <= 0xfffff)) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isUImm7Lsb00() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<5, 2>(Imm) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isUImm8Lsb00() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<6, 2>(Imm) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isUImm8Lsb000() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<5, 3>(Imm) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isSImm9Lsb0() const { return isBareSimmNLsb0<9>(); }

  bool isUImm9Lsb000() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<6, 3>(Imm) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isUImm10Lsb00NonZero() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isShiftedUInt<8, 2>(Imm) && (Imm != 0) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isSImm12() const {
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (!IsConstantImm)
      IsValid = false/*EMBERAsmParser::classifySymbolRef(getImm(), VK)*/;
    else
      IsValid = isInt<12>(Imm);
    return IsValid && ((IsConstantImm && VK == EMBERMCExpr::VK_EMBER_None) ||
                       VK == EMBERMCExpr::VK_EMBER_LO ||
                       VK == EMBERMCExpr::VK_EMBER_PCREL_LO ||
                       VK == EMBERMCExpr::VK_EMBER_TPREL_LO);
  }

  bool isSImm12Lsb0() const { return isBareSimmNLsb0<12>(); }

  bool isSImm13Lsb0() const { return isBareSimmNLsb0<13>(); }

  bool isSImm10Lsb0000NonZero() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && (Imm != 0) && isShiftedInt<6, 4>(Imm) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isUImm20LUI() const {
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (!IsConstantImm) {
      IsValid = false/*EMBERAsmParser::classifySymbolRef(getImm(), VK)*/;
      return IsValid && (VK == EMBERMCExpr::VK_EMBER_HI ||
                         VK == EMBERMCExpr::VK_EMBER_TPREL_HI);
    } else {
      return isUInt<20>(Imm) && (VK == EMBERMCExpr::VK_EMBER_None ||
                                 VK == EMBERMCExpr::VK_EMBER_HI ||
                                 VK == EMBERMCExpr::VK_EMBER_TPREL_HI);
    }
  }

  bool isUImm20AUIPC() const {
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsValid;
    if (!isImm())
      return false;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    if (!IsConstantImm) {
      IsValid = false/*EMBERAsmParser::classifySymbolRef(getImm(), VK)*/;
      return IsValid && (VK == EMBERMCExpr::VK_EMBER_PCREL_HI ||
                         VK == EMBERMCExpr::VK_EMBER_GOT_HI ||
                         VK == EMBERMCExpr::VK_EMBER_TLS_GOT_HI ||
                         VK == EMBERMCExpr::VK_EMBER_TLS_GD_HI);
    } else {
      return isUInt<20>(Imm) && (VK == EMBERMCExpr::VK_EMBER_None ||
                                 VK == EMBERMCExpr::VK_EMBER_PCREL_HI ||
                                 VK == EMBERMCExpr::VK_EMBER_GOT_HI ||
                                 VK == EMBERMCExpr::VK_EMBER_TLS_GOT_HI ||
                                 VK == EMBERMCExpr::VK_EMBER_TLS_GD_HI);
    }
  }

  bool isSImm21Lsb0JAL() const { return isBareSimmNLsb0<21>(); }

  bool isImmZero() const {
    if (!isImm())
      return false;
    int64_t Imm;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && (Imm == 0) && VK == EMBERMCExpr::VK_EMBER_None;
  }

  bool isSImm5Plus1() const {
    if (!isImm())
      return false;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    int64_t Imm;
    bool IsConstantImm = evaluateConstantImm(getImm(), Imm, VK);
    return IsConstantImm && isInt<5>(Imm - 1) &&
           VK == EMBERMCExpr::VK_EMBER_None;
  }

  /// getStartLoc - Gets location of the first token of this operand
  SMLoc getStartLoc() const override { return StartLoc; }
  /// getEndLoc - Gets location of the last token of this operand
  SMLoc getEndLoc() const override { return EndLoc; }

  unsigned getReg() const override {
    assert(Kind == KindTy::Register && "Invalid type access!");
    return Reg.RegNum.id();
  }

  StringRef getSysReg() const {
    assert(Kind == KindTy::UserRegister && "Invalid type access!");
    return StringRef(SysReg.Data, SysReg.Length);
  }

  const MCExpr *getImm() const {
    assert(Kind == KindTy::Immediate && "Invalid type access!");
    return Imm.Val;
  }

  StringRef getToken() const {
    assert(Kind == KindTy::Token && "Invalid type access!");
    return Tok;
  }

  unsigned getVType() const {
    assert(Kind == KindTy::VType && "Invalid type access!");
    return VType.Val;
  }

  void print(raw_ostream &OS) const override {
    auto RegName = [](unsigned Reg) {
//       if (Reg)
//         return EMBERInstPrinter::getRegisterName(Reg);
//       else
        return "noreg";
    };

    switch (Kind) {
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
      OS << "<sysreg: " << getSysReg() << '>';
      break;
    case KindTy::VType:
      OS << "<vtype: ";
//       EMBERVType::printVType(getVType(), OS);
      OS << '>';
      break;
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

  static std::unique_ptr<EMBEROperand> createReg(unsigned RegNo, SMLoc S)
  {
    auto Op = std::make_unique<EMBEROperand>(KindTy::Register);
    Op->Reg.RegNum = RegNo;
    Op->StartLoc = S;
//     Op->EndLoc = E;
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

  static std::unique_ptr<EMBEROperand> createUserReg(StringRef Str, SMLoc S, unsigned Encoding)
  {
    auto Op = std::make_unique<EMBEROperand>(KindTy::UserRegister);
    Op->SysReg.Data = Str.data();
    Op->SysReg.Length = Str.size();
    Op->SysReg.Encoding = Encoding;
    Op->StartLoc = S;
    return Op;
  }

  static std::unique_ptr<EMBEROperand> createVType(unsigned VTypeI, SMLoc S)
  {
    auto Op = std::make_unique<EMBEROperand>(KindTy::VType);
    Op->VType.Val = VTypeI;
    Op->StartLoc = S;
    return Op;
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    assert(Expr && "Expr shouldn't be null!");
    int64_t Imm = 0;
    EMBERMCExpr::VariantKind VK = EMBERMCExpr::VK_EMBER_None;
    bool IsConstant = evaluateConstantImm(Expr, Imm, VK);

    if (IsConstant)
      Inst.addOperand(MCOperand::createImm(Imm));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  // Used by the TableGen Code
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
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

  void addCSRSystemRegisterOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createImm(SysReg.Encoding));
  }

  void addVTypeIOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createImm(getVType()));
  }

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

bool EMBERAsmParser::generateImmOutOfRangeError(
    OperandVector &Operands, uint64_t ErrorInfo, int64_t Lower, int64_t Upper,
    Twine Msg = "immediate must be an integer in the range") {
  SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
  return Error(ErrorLoc, Msg + " [" + Twine(Lower) + ", " + Twine(Upper) + "]");
}

static std::string EMBERMnemonicSpellCheck(StringRef S,
                                           const FeatureBitset &FBS,
                                           unsigned VariantID = 0);
*/
bool EMBERAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) 
{
/*
  MCInst Inst;
  FeatureBitset MissingFeatures;

  auto Result = MatchInstructionImpl(Operands, Inst, ErrorInfo, MissingFeatures,
                                     MatchingInlineAsm);
  switch (Result) {
  default:
    break;
  case Match_Success:
    if (validateInstruction(Inst, Operands))
      return true;
    return processInstruction(Inst, IDLoc, Operands, Out);
  case Match_MissingFeature: {
    assert(MissingFeatures.any() && "Unknown missing features!");
    bool FirstFeature = true;
    std::string Msg = "instruction requires the following:";
    for (unsigned i = 0, e = MissingFeatures.size(); i != e; ++i) {
      if (MissingFeatures[i]) {
        Msg += FirstFeature ? " " : ", ";
        Msg += getSubtargetFeatureName(i);
        FirstFeature = false;
      }
    }
    return Error(IDLoc, Msg);
  }
  case Match_MnemonicFail: {
    FeatureBitset FBS = ComputeAvailableFeatures(getSTI().getFeatureBits());
    std::string Suggestion =
        EMBERMnemonicSpellCheck(((EMBEROperand &)*Operands[0]).getToken(), FBS);
    return Error(IDLoc, "unrecognized instruction mnemonic" + Suggestion);
  }
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
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
  if (Result > FIRST_TARGET_MATCH_RESULT_TY) {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U && ErrorInfo >= Operands.size())
      return Error(ErrorLoc, "too few operands for instruction");
  }

  switch (Result) {
  default:
    break;
  case Match_InvalidImmXLenLI:
    if (isRV64()) {
      SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
      return Error(ErrorLoc, "operand must be a constant 64-bit integer");
    }
    return generateImmOutOfRangeError(Operands, ErrorInfo,
                                      std::numeric_limits<int32_t>::min(),
                                      std::numeric_limits<uint32_t>::max());
  case Match_InvalidImmZero: {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[ErrorInfo]).getStartLoc();
    return Error(ErrorLoc, "immediate must be zero");
  }
  case Match_InvalidUImmLog2XLen:
    if (isRV64())
      return generateImmOutOfRangeError(Operands, ErrorInfo, 0, (1 << 6) - 1);
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
  }

*/
  llvm_unreachable("Unknown match type detected!");
}
/*
// Attempts to match Name as a register (either using the default name or
// alternative ABI names), setting RegNo to the matching register. Upon
// failure, returns true and sets RegNo to 0. If IsRV32E then registers
// x16-x31 will be rejected.
static bool matchRegisterNameHelper(bool IsRV32E, MCRegister &RegNo,
                                    StringRef Name) {
  RegNo = MatchRegisterName(Name);
  // The 16-/32- and 64-bit FPRs have the same asm name. Check that the initial
  // match always matches the 64-bit variant, and not the 16/32-bit one.
  assert(!(RegNo >= EMBER::F0_H && RegNo <= EMBER::F31_H));
  assert(!(RegNo >= EMBER::F0_F && RegNo <= EMBER::F31_F));
  // The default FPR register class is based on the tablegen enum ordering.
  static_assert(EMBER::F0_D < EMBER::F0_H, "FPR matching must be updated");
  static_assert(EMBER::F0_D < EMBER::F0_F, "FPR matching must be updated");
  if (RegNo == EMBER::NoRegister)
    RegNo = MatchRegisterAltName(Name);
  if (IsRV32E && RegNo >= EMBER::X16 && RegNo <= EMBER::X31)
    RegNo = EMBER::NoRegister;
  return RegNo == EMBER::NoRegister;
}
*/
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
  RegNo = 0;
  StringRef Name = getLexer().getTok().getIdentifier();

//   if (matchRegisterNameHelper(isRV32E(), (MCRegister &)RegNo, Name))
//     return MatchOperand_NoMatch;

  getParser().Lex(); // Eat identifier token.
  return MatchOperand_Success;
}
/*
OperandMatchResultTy EMBERAsmParser::parseRegister(OperandVector &Operands,
                                                   bool AllowParens) {
  SMLoc FirstS = getLoc();
  bool HadParens = false;
  AsmToken LParen;

  // If this is an LParen and a parenthesised register name is allowed, parse it
  // atomically.
  if (AllowParens && getLexer().is(AsmToken::LParen)) {
    AsmToken Buf[2];
    size_t ReadCount = getLexer().peekTokens(Buf);
    if (ReadCount == 2 && Buf[1].getKind() == AsmToken::RParen) {
      HadParens = true;
      LParen = getParser().getTok();
      getParser().Lex(); // Eat '('
    }
  }

  switch (getLexer().getKind()) {
  default:
    if (HadParens)
      getLexer().UnLex(LParen);
    return MatchOperand_NoMatch;
  case AsmToken::Identifier:
    StringRef Name = getLexer().getTok().getIdentifier();
    MCRegister RegNo;
    matchRegisterNameHelper(isRV32E(), RegNo, Name);

    if (RegNo == EMBER::NoRegister) {
      if (HadParens)
        getLexer().UnLex(LParen);
      return MatchOperand_NoMatch;
    }
    if (HadParens)
      Operands.push_back(EMBEROperand::createToken("(", FirstS, isRV64()));
    SMLoc S = getLoc();
    SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
    getLexer().Lex();
    Operands.push_back(EMBEROperand::createReg(RegNo, S, E, isRV64()));
  }

  if (HadParens) {
    getParser().Lex(); // Eat ')'
    Operands.push_back(EMBEROperand::createToken(")", getLoc(), isRV64()));
  }

  return MatchOperand_Success;
}

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

OperandMatchResultTy EMBERAsmParser::parseImmediate(OperandVector &Operands) {
  SMLoc S = getLoc();
  SMLoc E = SMLoc::getFromPointer(S.getPointer() - 1);
  const MCExpr *Res;

  switch (getLexer().getKind()) {
  default:
    return MatchOperand_NoMatch;
  case AsmToken::LParen:
  case AsmToken::Dot:
  case AsmToken::Minus:
  case AsmToken::Plus:
  case AsmToken::Exclaim:
  case AsmToken::Tilde:
  case AsmToken::Integer:
  case AsmToken::String:
  case AsmToken::Identifier:
    if (getParser().parseExpression(Res))
      return MatchOperand_ParseFail;
    break;
  case AsmToken::Percent:
    return parseOperandWithModifier(Operands);
  }

  Operands.push_back(EMBEROperand::createImm(Res, S, E, isRV64()));
  return MatchOperand_Success;
}

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
  if (getLexer().is(AsmToken::Identifier) &&
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

OperandMatchResultTy
EMBERAsmParser::parseMemOpBaseReg(OperandVector &Operands) {
  if (getLexer().isNot(AsmToken::LParen)) {
    Error(getLoc(), "expected '('");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat '('
  Operands.push_back(EMBEROperand::createToken("(", getLoc(), isRV64()));

  if (parseRegister(Operands) != MatchOperand_Success) {
    Error(getLoc(), "expected register");
    return MatchOperand_ParseFail;
  }

  if (getLexer().isNot(AsmToken::RParen)) {
    Error(getLoc(), "expected ')'");
    return MatchOperand_ParseFail;
  }

  getParser().Lex(); // Eat ')'
  Operands.push_back(EMBEROperand::createToken(")", getLoc(), isRV64()));

  return MatchOperand_Success;
}

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
bool EMBERAsmParser::parseOperand(OperandVector &Operands, StringRef Mnemonic) {
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
/*
  OperandMatchResultTy Result =
      MatchOperandParserImpl(Operands, Mnemonic, true);
  if (Result == MatchOperand_Success)
    return false;
  if (Result == MatchOperand_ParseFail)
    return true;

  // Attempt to parse token as a register.
  if (parseRegister(Operands, true) == MatchOperand_Success)
    return false;

  // Attempt to parse token as an immediate
  if (parseImmediate(Operands) == MatchOperand_Success) {
    // Parse memory base register if present
    if (getLexer().is(AsmToken::LParen))
      return parseMemOpBaseReg(Operands) != MatchOperand_Success;
    return false;
  }
*/

  // Finally we have exhausted all options and must declare defeat.
  Error(getLoc(), "unknown operand");
  return true;
}
bool EMBERAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                      StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) {
  // Ensure that if the instruction occurs when relaxation is enabled,
  // relocations are forced for the file. Ideally this would be done when there
  // is enough information to reliably determine if the instruction itself may
  // cause relaxations. Unfortunately instruction processing stage occurs in the
  // same pass as relocation emission, so it's too late to set a 'sticky bit'
  // for the entire file.

/*
  if (getSTI().getFeatureBits()[EMBER::FeatureRelax]) {
    auto *Assembler = getTargetStreamer().getStreamer().getAssemblerPtr();
    if (Assembler != nullptr) {
      EMBERAsmBackend &MAB =
          static_cast<EMBERAsmBackend &>(Assembler->getBackend());
      MAB.setForceRelocs();
    }
  }
*/


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
  while (getLexer().is(AsmToken::Comma)) {
    // Consume comma token
    getLexer().Lex();

    // Parse next operand
    if (parseOperand(Operands, Name))
      return true;

    ++OperandIdx;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    getParser().eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }

  getParser().Lex(); // Consume the EndOfStatement.
  return false;
}

/*
bool EMBERAsmParser::classifySymbolRef(const MCExpr *Expr,
                                       EMBERMCExpr::VariantKind &Kind) {
  Kind = EMBERMCExpr::VK_EMBER_None;

  if (const EMBERMCExpr *RE = dyn_cast<EMBERMCExpr>(Expr)) {
    Kind = RE->getKind();
    Expr = RE->getSubExpr();
  }

  MCValue Res;
  MCFixup Fixup;
  if (Expr->evaluateAsRelocatable(Res, nullptr, &Fixup))
    return Res.getRefKind() == EMBERMCExpr::VK_EMBER_None;
  return false;
}
*/
bool EMBERAsmParser::ParseDirective(AsmToken DirectiveID) {
  // This returns false if this function recognizes the directive
  // regardless of whether it is successfully handles or reports an
  // error. Otherwise it returns true to give the generic parser a
  // chance at recognizing it.
  StringRef IDVal = DirectiveID.getString();

//   if (IDVal == ".option")
//     return parseDirectiveOption();
//   else if (IDVal == ".attribute")
//     return parseDirectiveAttribute();

//   MCAsmParser& Parser = getParser();
// 
//   // Get the option token.
//   AsmToken Tok = Parser.getTok();
// 
//   Parser.Lex();
// 
//   AsmToken Tok2 = Parser.getTok();


  return true;
}
/*
bool EMBERAsmParser::parseDirectiveOption() {
  MCAsmParser &Parser = getParser();
  // Get the option token.
  AsmToken Tok = Parser.getTok();
  // At the moment only identifiers are supported.
  if (Tok.isNot(AsmToken::Identifier))
    return Error(Parser.getTok().getLoc(),
                 "unexpected token, expected identifier");

  StringRef Option = Tok.getIdentifier();

  if (Option == "push") {
    getTargetStreamer().emitDirectiveOptionPush();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    pushFeatureBits();
    return false;
  }

  if (Option == "pop") {
    SMLoc StartLoc = Parser.getTok().getLoc();
    getTargetStreamer().emitDirectiveOptionPop();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    if (popFeatureBits())
      return Error(StartLoc, ".option pop with no .option push");

    return false;
  }

  if (Option == "rvc") {
    getTargetStreamer().emitDirectiveOptionRVC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    setFeatureBits(EMBER::FeatureStdExtC, "c");
    return false;
  }

  if (Option == "norvc") {
    getTargetStreamer().emitDirectiveOptionNoRVC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    clearFeatureBits(EMBER::FeatureStdExtC, "c");
    return false;
  }

  if (Option == "pic") {
    getTargetStreamer().emitDirectiveOptionPIC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    ParserOptions.IsPicEnabled = true;
    return false;
  }

  if (Option == "nopic") {
    getTargetStreamer().emitDirectiveOptionNoPIC();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    ParserOptions.IsPicEnabled = false;
    return false;
  }

  if (Option == "relax") {
    getTargetStreamer().emitDirectiveOptionRelax();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    setFeatureBits(EMBER::FeatureRelax, "relax");
    return false;
  }

  if (Option == "norelax") {
    getTargetStreamer().emitDirectiveOptionNoRelax();

    Parser.Lex();
    if (Parser.getTok().isNot(AsmToken::EndOfStatement))
      return Error(Parser.getTok().getLoc(),
                   "unexpected token, expected end of statement");

    clearFeatureBits(EMBER::FeatureRelax, "relax");
    return false;
  }

  // Unknown option.
  Warning(Parser.getTok().getLoc(),
          "unknown option, expected 'push', 'pop', 'rvc', 'norvc', 'relax' or "
          "'norelax'");
  Parser.eatToEndOfStatement();
  return false;
}

/// parseDirectiveAttribute
///  ::= .attribute expression ',' ( expression | "string" )
///  ::= .attribute identifier ',' ( expression | "string" )
bool EMBERAsmParser::parseDirectiveAttribute() {
  MCAsmParser &Parser = getParser();
  int64_t Tag;
  SMLoc TagLoc;
  TagLoc = Parser.getTok().getLoc();
  if (Parser.getTok().is(AsmToken::Identifier)) {
    StringRef Name = Parser.getTok().getIdentifier();
    Optional<unsigned> Ret =
        ELFAttrs::attrTypeFromString(Name, EMBERAttrs::EMBERAttributeTags);
    if (!Ret.hasValue()) {
      Error(TagLoc, "attribute name not recognised: " + Name);
      return false;
    }
    Tag = Ret.getValue();
    Parser.Lex();
  } else {
    const MCExpr *AttrExpr;

    TagLoc = Parser.getTok().getLoc();
    if (Parser.parseExpression(AttrExpr))
      return true;

    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(AttrExpr);
    if (check(!CE, TagLoc, "expected numeric constant"))
      return true;

    Tag = CE->getValue();
  }

  if (Parser.parseToken(AsmToken::Comma, "comma expected"))
    return true;

  StringRef StringValue;
  int64_t IntegerValue = 0;
  bool IsIntegerValue = true;

  // Ember attributes have a string value if the tag number is odd
  // and an integer value if the tag number is even.
  if (Tag % 2)
    IsIntegerValue = false;

  SMLoc ValueExprLoc = Parser.getTok().getLoc();
  if (IsIntegerValue) {
    const MCExpr *ValueExpr;
    if (Parser.parseExpression(ValueExpr))
      return true;

    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(ValueExpr);
    if (!CE)
      return Error(ValueExprLoc, "expected numeric constant");
    IntegerValue = CE->getValue();
  } else {
    if (Parser.getTok().isNot(AsmToken::String))
      return Error(Parser.getTok().getLoc(), "expected string constant");

    StringValue = Parser.getTok().getStringContents();
    Parser.Lex();
  }

  if (Parser.parseToken(AsmToken::EndOfStatement,
                        "unexpected token in '.attribute' directive"))
    return true;

  if (Tag == EMBERAttrs::ARCH) {
    StringRef Arch = StringValue;
    if (Arch.consume_front("rv32"))
      clearFeatureBits(EMBER::Feature64Bit, "64bit");
    else if (Arch.consume_front("rv64"))
      setFeatureBits(EMBER::Feature64Bit, "64bit");
    else
      return Error(ValueExprLoc, "bad arch string " + Arch);

    // .attribute arch overrides the current architecture, so unset all
    // currently enabled extensions
    clearFeatureBits(EMBER::FeatureRV32E, "e");
    clearFeatureBits(EMBER::FeatureStdExtM, "m");
    clearFeatureBits(EMBER::FeatureStdExtA, "a");
    clearFeatureBits(EMBER::FeatureStdExtF, "f");
    clearFeatureBits(EMBER::FeatureStdExtD, "d");
    clearFeatureBits(EMBER::FeatureStdExtC, "c");
    clearFeatureBits(EMBER::FeatureStdExtB, "experimental-b");
    clearFeatureBits(EMBER::FeatureStdExtV, "experimental-v");
    clearFeatureBits(EMBER::FeatureExtZfh, "experimental-zfh");
    clearFeatureBits(EMBER::FeatureExtZba, "experimental-zba");
    clearFeatureBits(EMBER::FeatureExtZbb, "experimental-zbb");
    clearFeatureBits(EMBER::FeatureExtZbc, "experimental-zbc");
    clearFeatureBits(EMBER::FeatureExtZbe, "experimental-zbe");
    clearFeatureBits(EMBER::FeatureExtZbf, "experimental-zbf");
    clearFeatureBits(EMBER::FeatureExtZbm, "experimental-zbm");
    clearFeatureBits(EMBER::FeatureExtZbp, "experimental-zbp");
    clearFeatureBits(EMBER::FeatureExtZbproposedc, "experimental-zbproposedc");
    clearFeatureBits(EMBER::FeatureExtZbr, "experimental-zbr");
    clearFeatureBits(EMBER::FeatureExtZbs, "experimental-zbs");
    clearFeatureBits(EMBER::FeatureExtZbt, "experimental-zbt");
    clearFeatureBits(EMBER::FeatureExtZvamo, "experimental-zvamo");
    clearFeatureBits(EMBER::FeatureStdExtZvlsseg, "experimental-zvlsseg");

    while (!Arch.empty()) {
      bool DropFirst = true;
      if (Arch[0] == 'i')
        clearFeatureBits(EMBER::FeatureRV32E, "e");
      else if (Arch[0] == 'e')
        setFeatureBits(EMBER::FeatureRV32E, "e");
      else if (Arch[0] == 'g') {
        clearFeatureBits(EMBER::FeatureRV32E, "e");
        setFeatureBits(EMBER::FeatureStdExtM, "m");
        setFeatureBits(EMBER::FeatureStdExtA, "a");
        setFeatureBits(EMBER::FeatureStdExtF, "f");
        setFeatureBits(EMBER::FeatureStdExtD, "d");
      } else if (Arch[0] == 'm')
        setFeatureBits(EMBER::FeatureStdExtM, "m");
      else if (Arch[0] == 'a')
        setFeatureBits(EMBER::FeatureStdExtA, "a");
      else if (Arch[0] == 'f')
        setFeatureBits(EMBER::FeatureStdExtF, "f");
      else if (Arch[0] == 'd') {
        setFeatureBits(EMBER::FeatureStdExtF, "f");
        setFeatureBits(EMBER::FeatureStdExtD, "d");
      } else if (Arch[0] == 'c') {
        setFeatureBits(EMBER::FeatureStdExtC, "c");
      } else if (Arch[0] == 'b') {
        setFeatureBits(EMBER::FeatureStdExtB, "experimental-b");
      } else if (Arch[0] == 'v') {
        setFeatureBits(EMBER::FeatureStdExtV, "experimental-v");
      } else if (Arch[0] == 's' || Arch[0] == 'x' || Arch[0] == 'z') {
        StringRef Ext =
            Arch.take_until([](char c) { return ::isdigit(c) || c == '_'; });
        if (Ext == "zba")
          setFeatureBits(EMBER::FeatureExtZba, "experimental-zba");
        else if (Ext == "zbb")
          setFeatureBits(EMBER::FeatureExtZbb, "experimental-zbb");
        else if (Ext == "zbc")
          setFeatureBits(EMBER::FeatureExtZbc, "experimental-zbc");
        else if (Ext == "zbe")
          setFeatureBits(EMBER::FeatureExtZbe, "experimental-zbe");
        else if (Ext == "zbf")
          setFeatureBits(EMBER::FeatureExtZbf, "experimental-zbf");
        else if (Ext == "zbm")
          setFeatureBits(EMBER::FeatureExtZbm, "experimental-zbm");
        else if (Ext == "zbp")
          setFeatureBits(EMBER::FeatureExtZbp, "experimental-zbp");
        else if (Ext == "zbproposedc")
          setFeatureBits(EMBER::FeatureExtZbproposedc,
                         "experimental-zbproposedc");
        else if (Ext == "zbr")
          setFeatureBits(EMBER::FeatureExtZbr, "experimental-zbr");
        else if (Ext == "zbs")
          setFeatureBits(EMBER::FeatureExtZbs, "experimental-zbs");
        else if (Ext == "zbt")
          setFeatureBits(EMBER::FeatureExtZbt, "experimental-zbt");
        else if (Ext == "zfh")
          setFeatureBits(EMBER::FeatureExtZfh, "experimental-zfh");
        else if (Ext == "zvamo")
          setFeatureBits(EMBER::FeatureExtZvamo, "experimental-zvamo");
        else if (Ext == "zvlsseg")
          setFeatureBits(EMBER::FeatureStdExtZvlsseg, "experimental-zvlsseg");
        else
          return Error(ValueExprLoc, "bad arch string " + Ext);
        Arch = Arch.drop_until([](char c) { return ::isdigit(c) || c == '_'; });
        DropFirst = false;
      } else
        return Error(ValueExprLoc, "bad arch string " + Arch);

      if (DropFirst)
        Arch = Arch.drop_front(1);
      int major = 0;
      int minor = 0;
      Arch.consumeInteger(10, major);
      Arch.consume_front("p");
      Arch.consumeInteger(10, minor);
      Arch = Arch.drop_while([](char c) { return c == '_'; });
    }
  }

  if (IsIntegerValue)
    getTargetStreamer().emitAttribute(Tag, IntegerValue);
  else {
    if (Tag != EMBERAttrs::ARCH) {
      getTargetStreamer().emitTextAttribute(Tag, StringValue);
    } else {
      std::string formalArchStr = "rv32";
      if (getFeatureBits(EMBER::Feature64Bit))
        formalArchStr = "rv64";
      if (getFeatureBits(EMBER::FeatureRV32E))
        formalArchStr = (Twine(formalArchStr) + "e1p9").str();
      else
        formalArchStr = (Twine(formalArchStr) + "i2p0").str();

      if (getFeatureBits(EMBER::FeatureStdExtM))
        formalArchStr = (Twine(formalArchStr) + "_m2p0").str();
      if (getFeatureBits(EMBER::FeatureStdExtA))
        formalArchStr = (Twine(formalArchStr) + "_a2p0").str();
      if (getFeatureBits(EMBER::FeatureStdExtF))
        formalArchStr = (Twine(formalArchStr) + "_f2p0").str();
      if (getFeatureBits(EMBER::FeatureStdExtD))
        formalArchStr = (Twine(formalArchStr) + "_d2p0").str();
      if (getFeatureBits(EMBER::FeatureStdExtC))
        formalArchStr = (Twine(formalArchStr) + "_c2p0").str();
      if (getFeatureBits(EMBER::FeatureStdExtB))
        formalArchStr = (Twine(formalArchStr) + "_b0p93").str();
      if (getFeatureBits(EMBER::FeatureStdExtV))
        formalArchStr = (Twine(formalArchStr) + "_v0p10").str();
      if (getFeatureBits(EMBER::FeatureExtZfh))
        formalArchStr = (Twine(formalArchStr) + "_zfh0p1").str();
      if (getFeatureBits(EMBER::FeatureExtZba))
        formalArchStr = (Twine(formalArchStr) + "_zba0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbb))
        formalArchStr = (Twine(formalArchStr) + "_zbb0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbc))
        formalArchStr = (Twine(formalArchStr) + "_zbc0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbe))
        formalArchStr = (Twine(formalArchStr) + "_zbe0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbf))
        formalArchStr = (Twine(formalArchStr) + "_zbf0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbm))
        formalArchStr = (Twine(formalArchStr) + "_zbm0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbp))
        formalArchStr = (Twine(formalArchStr) + "_zbp0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbproposedc))
        formalArchStr = (Twine(formalArchStr) + "_zbproposedc0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbr))
        formalArchStr = (Twine(formalArchStr) + "_zbr0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbs))
        formalArchStr = (Twine(formalArchStr) + "_zbs0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZbt))
        formalArchStr = (Twine(formalArchStr) + "_zbt0p93").str();
      if (getFeatureBits(EMBER::FeatureExtZvamo))
        formalArchStr = (Twine(formalArchStr) + "_zvamo0p10").str();
      if (getFeatureBits(EMBER::FeatureStdExtZvlsseg))
        formalArchStr = (Twine(formalArchStr) + "_zvlsseg0p10").str();

      getTargetStreamer().emitTextAttribute(Tag, formalArchStr);
    }
  }

  return false;
}

void EMBERAsmParser::emitToStreamer(MCStreamer &S, const MCInst &Inst) {
  MCInst CInst;
  bool Res = compressInst(CInst, Inst, getSTI(), S.getContext());
  if (Res)
    ++EMBERNumInstrsCompressed;
  S.emitInstruction((Res ? CInst : Inst), getSTI());
}

void EMBERAsmParser::emitLoadImm(MCRegister DestReg, int64_t Value,
                                 MCStreamer &Out) {
  EMBERMatInt::InstSeq Seq = EMBERMatInt::generateInstSeq(Value, isRV64());

  MCRegister SrcReg = EMBER::X0;
  for (EMBERMatInt::Inst &Inst : Seq) {
    if (Inst.Opc == EMBER::LUI) {
      emitToStreamer(
          Out, MCInstBuilder(EMBER::LUI).addReg(DestReg).addImm(Inst.Imm));
    } else {
      emitToStreamer(
          Out, MCInstBuilder(Inst.Opc).addReg(DestReg).addReg(SrcReg).addImm(
                   Inst.Imm));
    }

    // Only the first instruction has X0 as its source.
    SrcReg = DestReg;
  }
}

void EMBERAsmParser::emitAuipcInstPair(MCOperand DestReg, MCOperand TmpReg,
                                       const MCExpr *Symbol,
                                       EMBERMCExpr::VariantKind VKHi,
                                       unsigned SecondOpcode, SMLoc IDLoc,
                                       MCStreamer &Out) {
  // A pair of instructions for PC-relative addressing; expands to
  //   TmpLabel: AUIPC TmpReg, VKHi(symbol)
  //             OP DestReg, TmpReg, %pcrel_lo(TmpLabel)
  MCContext &Ctx = getContext();

  MCSymbol *TmpLabel = Ctx.createNamedTempSymbol("pcrel_hi");
  Out.emitLabel(TmpLabel);

  const EMBERMCExpr *SymbolHi = EMBERMCExpr::create(Symbol, VKHi, Ctx);
  emitToStreamer(
      Out, MCInstBuilder(EMBER::AUIPC).addOperand(TmpReg).addExpr(SymbolHi));

  const MCExpr *RefToLinkTmpLabel =
      EMBERMCExpr::create(MCSymbolRefExpr::create(TmpLabel, Ctx),
                          EMBERMCExpr::VK_EMBER_PCREL_LO, Ctx);

  emitToStreamer(Out, MCInstBuilder(SecondOpcode)
                          .addOperand(DestReg)
                          .addOperand(TmpReg)
                          .addExpr(RefToLinkTmpLabel));
}

void EMBERAsmParser::emitLoadLocalAddress(MCInst &Inst, SMLoc IDLoc,
                                          MCStreamer &Out) {
  // The load local address pseudo-instruction "lla" is used in PC-relative
  // addressing of local symbols:
  //   lla rdest, symbol
  // expands to
  //   TmpLabel: AUIPC rdest, %pcrel_hi(symbol)
  //             ADDI rdest, rdest, %pcrel_lo(TmpLabel)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  emitAuipcInstPair(DestReg, DestReg, Symbol, EMBERMCExpr::VK_EMBER_PCREL_HI,
                    EMBER::ADDI, IDLoc, Out);
}

void EMBERAsmParser::emitLoadAddress(MCInst &Inst, SMLoc IDLoc,
                                     MCStreamer &Out) {
  // The load address pseudo-instruction "la" is used in PC-relative and
  // GOT-indirect addressing of global symbols:
  //   la rdest, symbol
  // expands to either (for non-PIC)
  //   TmpLabel: AUIPC rdest, %pcrel_hi(symbol)
  //             ADDI rdest, rdest, %pcrel_lo(TmpLabel)
  // or (for PIC)
  //   TmpLabel: AUIPC rdest, %got_pcrel_hi(symbol)
  //             Lx rdest, %pcrel_lo(TmpLabel)(rdest)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  unsigned SecondOpcode;
  EMBERMCExpr::VariantKind VKHi;
  if (ParserOptions.IsPicEnabled) {
    SecondOpcode = isRV64() ? EMBER::LD : EMBER::LW;
    VKHi = EMBERMCExpr::VK_EMBER_GOT_HI;
  } else {
    SecondOpcode = EMBER::ADDI;
    VKHi = EMBERMCExpr::VK_EMBER_PCREL_HI;
  }
  emitAuipcInstPair(DestReg, DestReg, Symbol, VKHi, SecondOpcode, IDLoc, Out);
}

void EMBERAsmParser::emitLoadTLSIEAddress(MCInst &Inst, SMLoc IDLoc,
                                          MCStreamer &Out) {
  // The load TLS IE address pseudo-instruction "la.tls.ie" is used in
  // initial-exec TLS model addressing of global symbols:
  //   la.tls.ie rdest, symbol
  // expands to
  //   TmpLabel: AUIPC rdest, %tls_ie_pcrel_hi(symbol)
  //             Lx rdest, %pcrel_lo(TmpLabel)(rdest)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  unsigned SecondOpcode = isRV64() ? EMBER::LD : EMBER::LW;
  emitAuipcInstPair(DestReg, DestReg, Symbol, EMBERMCExpr::VK_EMBER_TLS_GOT_HI,
                    SecondOpcode, IDLoc, Out);
}

void EMBERAsmParser::emitLoadTLSGDAddress(MCInst &Inst, SMLoc IDLoc,
                                          MCStreamer &Out) {
  // The load TLS GD address pseudo-instruction "la.tls.gd" is used in
  // global-dynamic TLS model addressing of global symbols:
  //   la.tls.gd rdest, symbol
  // expands to
  //   TmpLabel: AUIPC rdest, %tls_gd_pcrel_hi(symbol)
  //             ADDI rdest, rdest, %pcrel_lo(TmpLabel)
  MCOperand DestReg = Inst.getOperand(0);
  const MCExpr *Symbol = Inst.getOperand(1).getExpr();
  emitAuipcInstPair(DestReg, DestReg, Symbol, EMBERMCExpr::VK_EMBER_TLS_GD_HI,
                    EMBER::ADDI, IDLoc, Out);
}

void EMBERAsmParser::emitLoadStoreSymbol(MCInst &Inst, unsigned Opcode,
                                         SMLoc IDLoc, MCStreamer &Out,
                                         bool HasTmpReg) {
  // The load/store pseudo-instruction does a pc-relative load with
  // a symbol.
  //
  // The expansion looks like this
  //
  //   TmpLabel: AUIPC tmp, %pcrel_hi(symbol)
  //             [S|L]X    rd, %pcrel_lo(TmpLabel)(tmp)
  MCOperand DestReg = Inst.getOperand(0);
  unsigned SymbolOpIdx = HasTmpReg ? 2 : 1;
  unsigned TmpRegOpIdx = HasTmpReg ? 1 : 0;
  MCOperand TmpReg = Inst.getOperand(TmpRegOpIdx);
  const MCExpr *Symbol = Inst.getOperand(SymbolOpIdx).getExpr();
  emitAuipcInstPair(DestReg, TmpReg, Symbol, EMBERMCExpr::VK_EMBER_PCREL_HI,
                    Opcode, IDLoc, Out);
}

void EMBERAsmParser::emitPseudoExtend(MCInst &Inst, bool SignExtend,
                                      int64_t Width, SMLoc IDLoc,
                                      MCStreamer &Out) {
  // The sign/zero extend pseudo-instruction does two shifts, with the shift
  // amounts dependent on the XLEN.
  //
  // The expansion looks like this
  //
  //    SLLI rd, rs, XLEN - Width
  //    SR[A|R]I rd, rd, XLEN - Width
  MCOperand DestReg = Inst.getOperand(0);
  MCOperand SourceReg = Inst.getOperand(1);

  unsigned SecondOpcode = SignExtend ? EMBER::SRAI : EMBER::SRLI;
  int64_t ShAmt = (isRV64() ? 64 : 32) - Width;

  assert(ShAmt > 0 && "Shift amount must be non-zero.");

  emitToStreamer(Out, MCInstBuilder(EMBER::SLLI)
                          .addOperand(DestReg)
                          .addOperand(SourceReg)
                          .addImm(ShAmt));

  emitToStreamer(Out, MCInstBuilder(SecondOpcode)
                          .addOperand(DestReg)
                          .addOperand(DestReg)
                          .addImm(ShAmt));
}

void EMBERAsmParser::emitVMSGE(MCInst &Inst, unsigned Opcode, SMLoc IDLoc,
                               MCStreamer &Out) {
  if (Inst.getNumOperands() == 3) {
    // unmasked va >= x
    //
    //  pseudoinstruction: vmsge{u}.vx vd, va, x
    //  expansion: vmslt{u}.vx vd, va, x; vmnand.mm vd, vd, vd
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addReg(EMBER::NoRegister));
    emitToStreamer(Out, MCInstBuilder(EMBER::VMNAND_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0)));
  } else if (Inst.getNumOperands() == 4) {
    // masked va >= x, vd != v0
    //
    //  pseudoinstruction: vmsge{u}.vx vd, va, x, v0.t
    //  expansion: vmslt{u}.vx vd, va, x, v0.t; vmxor.mm vd, vd, v0
    assert(Inst.getOperand(0).getReg() != EMBER::V0 &&
           "The destination register should not be V0.");
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addOperand(Inst.getOperand(3)));
    emitToStreamer(Out, MCInstBuilder(EMBER::VMXOR_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addReg(EMBER::V0));
  } else if (Inst.getNumOperands() == 5 &&
             Inst.getOperand(0).getReg() == EMBER::V0) {
    // masked va >= x, vd == v0
    //
    //  pseudoinstruction: vmsge{u}.vx vd, va, x, v0.t, vt
    //  expansion: vmslt{u}.vx vt, va, x;  vmandnot.mm vd, vd, vt
    assert(Inst.getOperand(0).getReg() == EMBER::V0 &&
           "The destination register should be V0.");
    assert(Inst.getOperand(1).getReg() != EMBER::V0 &&
           "The temporary vector register should not be V0.");
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addOperand(Inst.getOperand(3))
                            .addOperand(Inst.getOperand(4)));
    emitToStreamer(Out, MCInstBuilder(EMBER::VMANDNOT_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1)));
  } else if (Inst.getNumOperands() == 5) {
    // masked va >= x, any vd
    //
    // pseudoinstruction: vmsge{u}.vx vd, va, x, v0.t, vt
    // expansion: vmslt{u}.vx vt, va, x; vmandnot.mm vt, v0, vt; vmandnot.mm vd,
    // vd, v0; vmor.mm vd, vt, vd
    assert(Inst.getOperand(1).getReg() != EMBER::V0 &&
           "The temporary vector register should not be V0.");
    emitToStreamer(Out, MCInstBuilder(Opcode)
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(2))
                            .addOperand(Inst.getOperand(3))
                            .addReg(EMBER::NoRegister));
    emitToStreamer(Out, MCInstBuilder(EMBER::VMANDNOT_MM)
                            .addOperand(Inst.getOperand(1))
                            .addReg(EMBER::V0)
                            .addOperand(Inst.getOperand(1)));
    emitToStreamer(Out, MCInstBuilder(EMBER::VMANDNOT_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(0))
                            .addReg(EMBER::V0));
    emitToStreamer(Out, MCInstBuilder(EMBER::VMOR_MM)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addOperand(Inst.getOperand(0)));
  }
}

bool EMBERAsmParser::checkPseudoAddTPRel(MCInst &Inst,
                                         OperandVector &Operands) {
  assert(Inst.getOpcode() == EMBER::PseudoAddTPRel && "Invalid instruction");
  assert(Inst.getOperand(2).isReg() && "Unexpected second operand kind");
  if (Inst.getOperand(2).getReg() != EMBER::X4) {
    SMLoc ErrorLoc = ((EMBEROperand &)*Operands[3]).getStartLoc();
    return Error(ErrorLoc, "the second input operand must be tp/x4 when using "
                           "%tprel_add modifier");
  }

  return false;
}

std::unique_ptr<EMBEROperand> EMBERAsmParser::defaultMaskRegOp() const {
  return EMBEROperand::createReg(EMBER::NoRegister, llvm::SMLoc(),
                                 llvm::SMLoc(), isRV64());
}

bool EMBERAsmParser::validateInstruction(MCInst &Inst,
                                         OperandVector &Operands) {
  if (Inst.getOpcode() == EMBER::PseudoVMSGEU_VX_M_T ||
      Inst.getOpcode() == EMBER::PseudoVMSGE_VX_M_T) {
    unsigned DestReg = Inst.getOperand(0).getReg();
    unsigned TempReg = Inst.getOperand(1).getReg();
    if (DestReg == TempReg) {
      SMLoc Loc = Operands.back()->getStartLoc();
      return Error(Loc, "The temporary vector register cannot be the same as "
                        "the destination register.");
    }
  }

  const MCInstrDesc &MCID = MII.get(Inst.getOpcode());
  unsigned Constraints =
      (MCID.TSFlags & EMBERII::ConstraintMask) >> EMBERII::ConstraintShift;
  if (Constraints == EMBERII::NoConstraint)
    return false;

  unsigned DestReg = Inst.getOperand(0).getReg();
  // Operands[1] will be the first operand, DestReg.
  SMLoc Loc = Operands[1]->getStartLoc();
  if (Constraints & EMBERII::VS2Constraint) {
    unsigned CheckReg = Inst.getOperand(1).getReg();
    if (DestReg == CheckReg)
      return Error(Loc, "The destination vector register group cannot overlap"
                        " the source vector register group.");
  }
  if ((Constraints & EMBERII::VS1Constraint) && (Inst.getOperand(2).isReg())) {
    unsigned CheckReg = Inst.getOperand(2).getReg();
    if (DestReg == CheckReg)
      return Error(Loc, "The destination vector register group cannot overlap"
                        " the source vector register group.");
  }
  if ((Constraints & EMBERII::VMConstraint) && (DestReg == EMBER::V0)) {
    // vadc, vsbc are special cases. These instructions have no mask register.
    // The destination register could not be V0.
    unsigned Opcode = Inst.getOpcode();
    if (Opcode == EMBER::VADC_VVM || Opcode == EMBER::VADC_VXM ||
        Opcode == EMBER::VADC_VIM || Opcode == EMBER::VSBC_VVM ||
        Opcode == EMBER::VSBC_VXM || Opcode == EMBER::VFMERGE_VFM ||
        Opcode == EMBER::VMERGE_VIM || Opcode == EMBER::VMERGE_VVM ||
        Opcode == EMBER::VMERGE_VXM)
      return Error(Loc, "The destination vector register group cannot be V0.");

    // Regardless masked or unmasked version, the number of operands is the
    // same. For example, "viota.m v0, v2" is "viota.m v0, v2, NoRegister"
    // actually. We need to check the last operand to ensure whether it is
    // masked or not.
    unsigned CheckReg = Inst.getOperand(Inst.getNumOperands() - 1).getReg();
    assert((CheckReg == EMBER::V0 || CheckReg == EMBER::NoRegister) &&
           "Unexpected register for mask operand");

    if (DestReg == CheckReg)
      return Error(Loc, "The destination vector register group cannot overlap"
                        " the mask register.");
  }
  return false;
}

bool EMBERAsmParser::processInstruction(MCInst &Inst, SMLoc IDLoc,
                                        OperandVector &Operands,
                                        MCStreamer &Out) {
  Inst.setLoc(IDLoc);

  switch (Inst.getOpcode()) {
  default:
    break;
  case EMBER::PseudoLI: {
    MCRegister Reg = Inst.getOperand(0).getReg();
    const MCOperand &Op1 = Inst.getOperand(1);
    if (Op1.isExpr()) {
      // We must have li reg, %lo(sym) or li reg, %pcrel_lo(sym) or similar.
      // Just convert to an addi. This allows compatibility with gas.
      emitToStreamer(Out, MCInstBuilder(EMBER::ADDI)
                              .addReg(Reg)
                              .addReg(EMBER::X0)
                              .addExpr(Op1.getExpr()));
      return false;
    }
    int64_t Imm = Inst.getOperand(1).getImm();
    // On RV32 the immediate here can either be a signed or an unsigned
    // 32-bit number. Sign extension has to be performed to ensure that Imm
    // represents the expected signed 64-bit number.
    if (!isRV64())
      Imm = SignExtend64<32>(Imm);
    emitLoadImm(Reg, Imm, Out);
    return false;
  }
  case EMBER::PseudoLLA:
    emitLoadLocalAddress(Inst, IDLoc, Out);
    return false;
  case EMBER::PseudoLA:
    emitLoadAddress(Inst, IDLoc, Out);
    return false;
  case EMBER::PseudoLA_TLS_IE:
    emitLoadTLSIEAddress(Inst, IDLoc, Out);
    return false;
  case EMBER::PseudoLA_TLS_GD:
    emitLoadTLSGDAddress(Inst, IDLoc, Out);
    return false;
  case EMBER::PseudoLB:
    emitLoadStoreSymbol(Inst, EMBER::LB, IDLoc, Out, false);
    return false;
  case EMBER::PseudoLBU:
    emitLoadStoreSymbol(Inst, EMBER::LBU, IDLoc, Out, false);
    return false;
  case EMBER::PseudoLH:
    emitLoadStoreSymbol(Inst, EMBER::LH, IDLoc, Out, false);
    return false;
  case EMBER::PseudoLHU:
    emitLoadStoreSymbol(Inst, EMBER::LHU, IDLoc, Out, false);
    return false;
  case EMBER::PseudoLW:
    emitLoadStoreSymbol(Inst, EMBER::LW, IDLoc, Out, false);
    return false;
  case EMBER::PseudoLWU:
    emitLoadStoreSymbol(Inst, EMBER::LWU, IDLoc, Out, false);
    return false;
  case EMBER::PseudoLD:
    emitLoadStoreSymbol(Inst, EMBER::LD, IDLoc, Out, false);
    return false;
  case EMBER::PseudoFLH:
    emitLoadStoreSymbol(Inst, EMBER::FLH, IDLoc, Out, true);
    return false;
  case EMBER::PseudoFLW:
    emitLoadStoreSymbol(Inst, EMBER::FLW, IDLoc, Out, true);
    return false;
  case EMBER::PseudoFLD:
    emitLoadStoreSymbol(Inst, EMBER::FLD, IDLoc, Out, true);
    return false;
  case EMBER::PseudoSB:
    emitLoadStoreSymbol(Inst, EMBER::SB, IDLoc, Out, true);
    return false;
  case EMBER::PseudoSH:
    emitLoadStoreSymbol(Inst, EMBER::SH, IDLoc, Out, true);
    return false;
  case EMBER::PseudoSW:
    emitLoadStoreSymbol(Inst, EMBER::SW, IDLoc, Out, true);
    return false;
  case EMBER::PseudoSD:
    emitLoadStoreSymbol(Inst, EMBER::SD, IDLoc, Out, true);
    return false;
  case EMBER::PseudoFSH:
    emitLoadStoreSymbol(Inst, EMBER::FSH, IDLoc, Out, true);
    return false;
  case EMBER::PseudoFSW:
    emitLoadStoreSymbol(Inst, EMBER::FSW, IDLoc, Out, true);
    return false;
  case EMBER::PseudoFSD:
    emitLoadStoreSymbol(Inst, EMBER::FSD, IDLoc, Out, true);
    return false;
  case EMBER::PseudoAddTPRel:
    if (checkPseudoAddTPRel(Inst, Operands))
      return true;
    break;
  case EMBER::PseudoSEXT_B:
    emitPseudoExtend(Inst, true, 8, IDLoc, Out);
    return false;
  case EMBER::PseudoSEXT_H:
    emitPseudoExtend(Inst, true, 16, IDLoc, Out);
    return false;
  case EMBER::PseudoZEXT_H:
    emitPseudoExtend(Inst, false, 16, IDLoc, Out);
    return false;
  case EMBER::PseudoZEXT_W:
    emitPseudoExtend(Inst, false, 32, IDLoc, Out);
    return false;
  case EMBER::PseudoVMSGEU_VX:
  case EMBER::PseudoVMSGEU_VX_M:
  case EMBER::PseudoVMSGEU_VX_M_T:
    emitVMSGE(Inst, EMBER::VMSLTU_VX, IDLoc, Out);
    return false;
  case EMBER::PseudoVMSGE_VX:
  case EMBER::PseudoVMSGE_VX_M:
  case EMBER::PseudoVMSGE_VX_M_T:
    emitVMSGE(Inst, EMBER::VMSLT_VX, IDLoc, Out);
    return false;
  case EMBER::PseudoVMSGE_VI:
  case EMBER::PseudoVMSLT_VI: {
    // These instructions are signed and so is immediate so we can subtract one
    // and change the opcode.
    int64_t Imm = Inst.getOperand(2).getImm();
    unsigned Opc = Inst.getOpcode() == EMBER::PseudoVMSGE_VI ? EMBER::VMSGT_VI
                                                             : EMBER::VMSLE_VI;
    emitToStreamer(Out, MCInstBuilder(Opc)
                            .addOperand(Inst.getOperand(0))
                            .addOperand(Inst.getOperand(1))
                            .addImm(Imm - 1)
                            .addOperand(Inst.getOperand(3)));
    return false;
  }
  case EMBER::PseudoVMSGEU_VI:
  case EMBER::PseudoVMSLTU_VI: {
    int64_t Imm = Inst.getOperand(2).getImm();
    // Unsigned comparisons are tricky because the immediate is signed. If the
    // immediate is 0 we can't just subtract one. vmsltu.vi v0, v1, 0 is always
    // false, but vmsle.vi v0, v1, -1 is always true. Instead we use
    // vmsne v0, v1, v1 which is always false.
    if (Imm == 0) {
      unsigned Opc = Inst.getOpcode() == EMBER::PseudoVMSGEU_VI
                         ? EMBER::VMSEQ_VV
                         : EMBER::VMSNE_VV;
      emitToStreamer(Out, MCInstBuilder(Opc)
                              .addOperand(Inst.getOperand(0))
                              .addOperand(Inst.getOperand(1))
                              .addOperand(Inst.getOperand(1))
                              .addOperand(Inst.getOperand(3)));
    } else {
      // Other immediate values can subtract one like signed.
      unsigned Opc = Inst.getOpcode() == EMBER::PseudoVMSGEU_VI
                         ? EMBER::VMSGTU_VI
                         : EMBER::VMSLEU_VI;
      emitToStreamer(Out, MCInstBuilder(Opc)
                              .addOperand(Inst.getOperand(0))
                              .addOperand(Inst.getOperand(1))
                              .addImm(Imm - 1)
                              .addOperand(Inst.getOperand(3)));
    }

    return false;
  }
  }

  emitToStreamer(Out, Inst);
  return false;
}
*/
extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeEMBERAsmParser() {
  RegisterMCAsmParser<EMBERAsmParser> X(getTheEMBER32Target());
}
