//===- AVR.cpp ------------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// AVR is a Harvard-architecture 8-bit micrcontroller designed for small
// baremetal programs. All AVR-family processors have 32 8-bit registers.
// The tiniest AVR has 32 byte RAM and 1 KiB program memory, and the largest
// one supports up to 2^24 data address space and 2^22 code address space.
//
// Since it is a baremetal programming, there's usually no loader to load
// ELF files on AVRs. You are expected to link your program against address
// 0 and pull out a .text section from the result using objcopy, so that you
// can write the linked code to on-chip flush memory. You can do that with
// the following commands:
//
//   ld.lld -Ttext=0 -o foo foo.o
//   objcopy -O binary --only-section=.text foo output.bin
//
// Note that the current AVR support is very preliminary so you can't
// link any useful program yet, though.
//
//===----------------------------------------------------------------------===//

#include "Error.h"
#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "llvm/Object/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
uint16_t calculateForLDI(uint16_t X, int16_t Val) {
  return (X & 0xf0f0) | (Val & 0xf) | ((Val << 4) & 0xf00);
}

// All PC-relative instructions need to be adjusted by -2 IIRC to account for
// the size of the current instruction.
void adjustRelativeBranch(int16_t &Val) {
  Val -= 2; // Branch instructions add 2 to the PC.
}

// Adjusts a program memory address. This is a simple right-shift.
void adjustPM(int16_t &Val) {
  Val >>= 1; // AVR addresses commands as words.
}

void adjustNeg(int16_t &Val) { Val *= -1; }

class AVR final : public TargetInfo {
public:
  RelExpr getRelExpr(uint32_t Type, const SymbolBody &S, const InputFile &File,
                     const uint8_t *Loc) const override;
  void relocateOne(uint8_t *Loc, uint32_t Type, uint64_t Val) const override;
};
} // namespace

RelExpr AVR::getRelExpr(uint32_t Type, const SymbolBody &S,
                        const InputFile &File, const uint8_t *Loc) const {
  switch (Type) {
  case R_AVR_7_PCREL:
  case R_AVR_13_PCREL:
    return R_PC;
  case R_AVR_LO8_LDI:
  case R_AVR_LDI:
  case R_AVR_6:
  case R_AVR_6_ADIW:
  case R_AVR_HI8_LDI:
  case R_AVR_HH8_LDI:
  case R_AVR_MS8_LDI:
  case R_AVR_LO8_LDI_NEG:
  case R_AVR_HI8_LDI_NEG:
  case R_AVR_HH8_LDI_NEG:
  case R_AVR_MS8_LDI_NEG:
  case R_AVR_LO8_LDI_GS:
  case R_AVR_LO8_LDI_PM:
  case R_AVR_HI8_LDI_GS:
  case R_AVR_HI8_LDI_PM:
  case R_AVR_HH8_LDI_PM:
  case R_AVR_LO8_LDI_PM_NEG:
  case R_AVR_HI8_LDI_PM_NEG:
  case R_AVR_HH8_LDI_PM_NEG:
  case R_AVR_8:
  case R_AVR_8_LO8:
  case R_AVR_8_HI8:
  case R_AVR_8_HLO8:
  case R_AVR_CALL:
  case R_AVR_16:
  case R_AVR_16_PM:
  case R_AVR_LDS_STS_16:
  case R_AVR_PORT6:
  case R_AVR_PORT5:
    return R_ABS;
  default:
    error(toString(&File) + ": unknown relocation type: " + toString(Type));
    return R_HINT;
  }
}

void AVR::relocateOne(uint8_t *Loc, uint32_t Type, uint64_t Val) const {
  int16_t SRel = Val;
  uint16_t X = read16le(Loc);
  switch (Type) {
  case R_AVR_7_PCREL:
    adjustRelativeBranch(SRel);
    X = (X & 0xfc07) | (((SRel >> 1) << 3) & 0x3f8);
    write16le(Loc, X);
    break;
  case R_AVR_13_PCREL:
    adjustRelativeBranch(SRel);
    adjustPM(SRel);
    X = (X & 0xf000) | (SRel & 0xfff);
    write16le(Loc, X);
    break;
  case R_AVR_LO8_LDI:
  case R_AVR_LDI:
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_6:
    X = (X & 0xd3f8) | ((SRel & 7) | ((SRel & (3 << 3)) << 7)
                     | ((SRel & (1 << 5)) << 8));
    write16le(Loc, X);
    break;
  case R_AVR_6_ADIW:
    X = (X & 0xff30) | (SRel & 0xf) | ((SRel & 0x30) << 2);
    write16le(Loc, X);
    break;
  case R_AVR_HI8_LDI:
    SRel = (SRel >> 8) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HH8_LDI:
    SRel = (SRel >> 16) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_MS8_LDI:
    SRel = (SRel >> 24) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_LO8_LDI_NEG:
    adjustNeg(SRel);
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HI8_LDI_NEG:
    adjustNeg(SRel);
    SRel = (SRel >> 8) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HH8_LDI_NEG:
    adjustNeg(SRel);
    SRel = (SRel >> 16) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_MS8_LDI_NEG:
    adjustNeg(SRel);
    SRel = (SRel >> 24) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_LO8_LDI_GS:
  case R_AVR_LO8_LDI_PM:
    adjustPM(SRel);
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HI8_LDI_GS:
  case R_AVR_HI8_LDI_PM:
    adjustPM(SRel);
    SRel = (SRel >> 8) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HH8_LDI_PM:
    adjustPM(SRel);
    SRel = (SRel >> 16) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_LO8_LDI_PM_NEG:
    adjustNeg(SRel);
    adjustPM(SRel);
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HI8_LDI_PM_NEG:
    adjustNeg(SRel);
    adjustPM(SRel);
    SRel = (SRel >> 8) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_HH8_LDI_PM_NEG:
    adjustNeg(SRel);
    adjustPM(SRel);
    SRel = (SRel >> 16) & 0xff;
    write16le(Loc, calculateForLDI(X, SRel));
    break;
  case R_AVR_8:
    write16le(Loc, SRel & 0x000000ff);
    break;
  case R_AVR_8_LO8:
    write16le(Loc, SRel & 0xffffff);
    break;
  case R_AVR_8_HI8:
    write16le(Loc, (SRel >> 8) & 0xffffff);
    break;
  case R_AVR_8_HLO8:
    write16le(Loc, (SRel >> 16) & 0xffffff);
    break;
  case R_AVR_CALL:
    adjustPM(SRel);
    X |= ((SRel & 0x10000) | ((SRel << 3) & 0x1f00000)) >> 16;
    write16le(Loc, X);
    write16le(Loc + 2, SRel & 0xffff);
    break;
  case R_AVR_16:
    write16le(Loc, SRel & 0x00ffff);
    break;
  case R_AVR_16_PM:
    adjustPM(SRel);
    write16le(Loc, SRel & 0x00ffff);
    break;
  case R_AVR_LDS_STS_16:
    SRel = SRel & 0x7f;
    X |= (SRel & 0x0f) | ((SRel & 0x30) << 5) | ((SRel & 0x40) << 2);
    write16le(Loc, X);
    break;
  case R_AVR_PORT6:
    X = (X & 0xf9f0) | ((SRel & 0x30) << 5) | (SRel & 0x0f);
    write16le(Loc, X);
    break;
  case R_AVR_PORT5:
    X = (X & 0xff07) | ((SRel & 0x1f) << 3);
    write16le(Loc, X);
    break;
  default:
    error(getErrorLocation(Loc) + "unrecognized reloc " + toString(Type));
  }
}

TargetInfo *elf::getAVRTargetInfo() {
  static AVR Target;
  return &Target;
}
