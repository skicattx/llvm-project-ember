//===- EMBER.cpp --------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace 
{
    class EMBER final : public TargetInfo 
    {
    public:
        EMBER();
        RelExpr getRelExpr(RelType type, const Symbol &s, const uint8_t *loc) const override;
        void writePlt(uint8_t *buf, const Symbol &sym, uint64_t pltEntryAddr) const override;
        void relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const override;
    };
} // namespace


EMBER::EMBER() 
{
    /*
    copyRel = R_SPARC_COPY;
    gotRel = R_SPARC_GLOB_DAT;
    noneRel = R_SPARC_NONE;
    pltRel = R_SPARC_JMP_SLOT;
    relativeRel = R_SPARC_RELATIVE;
    symbolicRel = R_SPARC_64;
    pltEntrySize = 32;
    pltHeaderSize = 4 * pltEntrySize;

    defaultCommonPageSize = 8192;
    defaultMaxPageSize = 0x100000;
    defaultImageBase = 0x100000;
    */
}

RelExpr EMBER::getRelExpr(RelType type, const Symbol &s, const uint8_t *loc) const 
{
    switch (type) 
    {
        case R_EMBER_NONE:
            return R_NONE;
        case R_EMBER_32:
            return R_ABS; // Need custom? // For now this is "unknown" 32-bit relocation (will be saved in ELF) probably debug/dwarf related?
        case R_EMBER_32_PCREL:
            return R_PC;  // Need custom? // For now this is "unknown" 32-bit (PC-rel) relocation (will be saved in ELF) probably debug/dwarf related?
        case R_EMBER_BRANCH:
            return R_PC; // Need custom?
        case R_EMBER_LDI_LABEL_ADDR_LO:
            return R_ABS; // Need custom?
        case R_EMBER_LDI_LABEL_ADDR_HI:
            return R_ABS; // Need custom?
        default:
            error(getErrorLocation(loc) + "unknown relocation (" + Twine(type) +") against symbol " + toString(s));

        return R_NONE;
    }
}

void EMBER::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const 
{
    switch (rel.type) 
    {
        case R_EMBER_32: // For now this is "unknown" 32-bit relocation (will be saved in ELF) probably debug/dwarf related?
            break;
        case R_EMBER_32_PCREL: // For now this is "unknown" 32-bit (PC-rel) relocation (will be saved in ELF) probably debug/dwarf related?
            break;
        case R_EMBER_BRANCH:
            checkInt(loc, ((int64_t)val) >> 2, 22, rel);
            write32le(loc, (read32le(loc) & ~0x003fffff) | ( (((int64_t)val)>>2) & 0x003fffff));
            break;
        case R_EMBER_LDI_LABEL_ADDR_LO:
            checkUInt(loc, val, 32, rel);
            write32le(loc, (read32le(loc) & ~0x0000ffff) | (val & 0x0000ffff));
            break;
        case R_EMBER_LDI_LABEL_ADDR_HI:
            checkUInt(loc, val, 32, rel);
            write32le(loc, (read32le(loc) & ~0x0000ffff) | ( (val>>16) & 0x0000ffff));
            break;
        case R_EMBER_NONE:
            llvm_unreachable("unknown relocation");
        default:
            llvm_unreachable("unknown relocation");
    }
}

void EMBER::writePlt(uint8_t *buf, const Symbol & /*sym*/, uint64_t pltEntryAddr) const 
{
    // Need to encode some Ember instructions for header?
  report_fatal_error("writePlt Not Implemented");

    const uint8_t pltData[] = 
    {
      0x03, 0x00, 0x00, 0x00, // sethi   (. - .PLT0), %g1
      0x30, 0x68, 0x00, 0x00, // ba,a    %xcc, .PLT1
      0x01, 0x00, 0x00, 0x00, // nop
      0x01, 0x00, 0x00, 0x00, // nop
      0x01, 0x00, 0x00, 0x00, // nop
      0x01, 0x00, 0x00, 0x00, // nop
      0x01, 0x00, 0x00, 0x00, // nop
      0x01, 0x00, 0x00, 0x00  // nop
    };
    memcpy(buf, pltData, sizeof(pltData));

    uint64_t off = pltEntryAddr - in.plt->getVA();
    relocateNoSym(buf, R_SPARC_22, off);
    relocateNoSym(buf + 4, R_SPARC_WDISP19, -(off + 4 - pltEntrySize));
}

TargetInfo *elf::getEMBERTargetInfo() 
{
  static EMBER target;
  return &target;
}
