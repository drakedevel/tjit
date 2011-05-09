#include <assert.h>

#include "MASM.hh"

MASM::Register MASM::REG_NONE(0);
MASM::Register MASM::RAX(0);
MASM::Register MASM::RCX(1);
MASM::Register MASM::RDX(2);
MASM::Register MASM::RBX(3);
MASM::Register MASM::RSP(4);
MASM::Register MASM::RBP(5);
MASM::Register MASM::RSI(6);
MASM::Register MASM::RDI(7);
MASM::Register MASM::R8(8);
MASM::Register MASM::R9(9);
MASM::Register MASM::R10(10);
MASM::Register MASM::R11(11);
MASM::Register MASM::R12(12);
MASM::Register MASM::R13(13);
MASM::Register MASM::R14(14);
MASM::Register MASM::R15(15);

MASM::MASM(void *buffer)
{
    mBase = mPointer = buffer;
}

MASM::Label MASM::label()
{
    return Label((char *)mPointer - (char *)mBase);
}

void MASM::move64(Register dest, uint64_t immed)
{
    uint8_t r = dest.getNumber();
    doREX(REG_NONE, dest, true);
    write8(0xB8u + (r & 0x7));
    write64(immed);
}

void MASM::move64(Register dest, Register src)
{
    doREX(src, dest, true);
    write8(0x89);
    doModRM(src, dest);
}

void MASM::call(Register reg)
{
    write8(0xFFu);
    doModRM(Register(2), reg);
}

void MASM::compare8(Location where, uint8_t value)
{
    doREX(Register(7), where, false);
    write8(0x80u);
    doModRMSIB(Register(7), where);
    if (where.getOffset())
        write32(where.getOffset()); // FIXME:
    write8(value);
}

void MASM::compare64(Register a, Register b)
{
    doREX(b, a, true);
    write8(0x39u);
    doModRM(b, a);
}

void MASM::load64(Register dest, Location source)
{
    doREX(dest, source, true);
    write8(0x8Bu);
    doModRMSIB(dest, source);
    assert(!source.getOffset());
}

void MASM::store8(Location where, uint8_t value)
{
    doREX(REG_NONE, where, false, true);
    write8(0xC6u);
    doModRMSIB(REG_NONE, where);
    if (where.getOffset())
        write32(where.getOffset()); // FIXME:
    write8(value);
}

MASM::Jump MASM::jump32()
{
    write8(0xE9);
    unsigned int offsetBase = (char *)mPointer - (char *)mBase;
    write32(0);
    unsigned int relativeTo = (char *)mPointer - (char *)mBase;
    return Jump(relativeTo, offsetBase);
}

MASM::Jump MASM::jump32(Condition cond)
{
    write8(0x0F);
    write8(0x80 + cond);
    unsigned int offsetBase = (char *)mPointer - (char *)mBase;
    write32(0);
    unsigned int relativeTo = (char *)mPointer - (char *)mBase;
    return Jump(relativeTo, offsetBase);
}

void MASM::jumpIndirect(Register reg)
{
    doREX(REG_NONE, reg, false);
    write8(0xFFu);
    doModRM(Register(4), reg);
}

void MASM::jumpReallyIndirect(Location where)
{
    doREX(REG_NONE, where, false);
    write8(0xFFu);
    doModRMSIB(Register(4), where);
    assert(!where.getOffset());
}

void MASM::add32(Register dest, uint32_t imm)
{
    doREX(0, 0, dest.getNumber(), true);
    write8(0x81u);
    doModRM(REG_NONE, dest);
    write32(imm);
}

void MASM::add64(Register dest, Register source)
{
    doREX(dest, source, true);
    write8(0x03u);
    doModRM(dest, source);
}

void MASM::ret()
{
    write8(0xC3u);
}

void MASM::die()
{
    write8(0xCCu);
}

void MASM::link(Jump j, Label l)
{
    int offset = l.getOffset() - j.getRelativeBase();
    *((int *)((char *)mBase + j.getOffsetBase())) = offset;
}

void MASM::write8(uint8_t byte)
{
    uint8_t *ptr = (uint8_t *)mPointer;
    *(ptr++) = byte;
    mPointer = ptr;
}

void MASM::write32(uint32_t dword)
{
    uint32_t *ptr = (uint32_t *)mPointer;
    *(ptr++) = dword;
    mPointer = ptr;
}

void MASM::write64(uint64_t qword)
{
    uint64_t *ptr = (uint64_t *)mPointer;
    *(ptr++) = qword;
    mPointer = ptr;
}

void MASM::doModRM(Register rr, Register rmr)
{
    uint8_t mod = MOD_DIRECT;
    uint8_t r = rr.getNumber();
    uint8_t rm = rmr.getNumber();

    doModRM(mod, r, rm);
}

void MASM::doModRMSIB(Register rr, Location rml)
{
    uint8_t mod = MOD_DEREF;
    if (rml.getOffset() != 0)
        mod = MOD_DEREFPLUS32;
    uint8_t r = rr.getNumber();
    uint8_t b = rml.getBase().getNumber();
    if (rml.getMultiplier() > 0) {
        uint8_t s;
        switch (rml.getMultiplier()) {
        case 8:
            s = SS_MULT8;
            break;
        case 4:
            s = SS_MULT4;
            break;
        case 2:
            s = SS_MULT2;
            break;
        case 1:
            s = SS_MULT1;
            break;
        default:
            assert(0);
        }
        uint8_t i = rml.getOffsetReg().getNumber();

        doModRM(mod, r, 3);
        doSIB(s, i, b);
    } else {
        doModRM(mod, r, b);
    }
}

void MASM::doModRM(uint8_t mod, uint8_t r, uint8_t rm)
{
    uint8_t modrm = ((mod & 0x3) << 6) |
                    ((r & 0x7) << 3) |
                    (rm & 0x7);
    write8(modrm);
}

void MASM::doSIB(uint8_t s, uint8_t i, uint8_t b)
{
    uint8_t sib = ((s & 0x3) << 6) |
                  ((i & 0x7) << 3) |
                  (b & 0x7);
    write8(sib);
}

void MASM::doREX(Register rr, Register rmr, bool w, bool force)
{
    uint8_t r = rr.getNumber();
    uint8_t rm = rmr.getNumber();

    doREX(r, 0, rm, w, force);
}

void MASM::doREX(Register rr, Location rmr, bool w, bool force)
{
    uint8_t r = rr.getNumber();
    uint8_t x = rmr.getMultiplier() > 0 ? rmr.getOffsetReg().getNumber() : 0;
    uint8_t b = rmr.getBase().getNumber();

    doREX(r, x, b, w, force);
}

void MASM::doREX(uint8_t r, uint8_t x, uint8_t rm, bool w, bool force)
{
    uint8_t rex = (1 << 6) |
                  (w ? 1 << 3 : 0) |
                  ((r >> 3 & 1) << 2) |
                  ((x >> 3 & 1) << 1) |
                  (rm >> 3 & 1);
    if (force || r || x || rm || w)
        write8(rex);
}

MASM::Jump::Jump(unsigned int relativeTo, unsigned int offsetBase) :
    mRelativeBase(relativeTo),
    mOffsetBase(offsetBase)
{
}

unsigned int MASM::Jump::getRelativeBase()
{
    return mRelativeBase;
}

unsigned int MASM::Jump::getOffsetBase()
{
    return mOffsetBase;
}

MASM::Label::Label(unsigned int offset) :
    mOffset(offset)
{
}

unsigned int MASM::Label::getOffset()
{
    return mOffset;
}

MASM::Register::Register(unsigned int num) :
    mNumber(num)
{
}

unsigned int MASM::Register::getNumber()
{
    return mNumber;
}

MASM::Location::Location(MASM::Register reg, MASM::Register offsetReg, int multiplier, int offset) :
    mBase(reg),
    mOffsetReg(offsetReg),
    mMultiplier(multiplier),
    mOffset(offset)
{
}

MASM::Register MASM::Location::getBase()
{
    return mBase;
}

MASM::Register MASM::Location::getOffsetReg()
{
    return mOffsetReg;
}

int MASM::Location::getMultiplier()
{
    return mMultiplier;
}

int MASM::Location::getOffset()
{
    return mOffset;
}
