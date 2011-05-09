#ifndef MASM_HH__
#define MASM_HH__

#include <stdint.h>

class MASM
{
public:
    class Jump;
    class Label;
    class Location;
    class Register;
    enum Condition {
        COND_OVERFLOW = 0,
        COND_NOT_OVERFLOW,
        COND_BELOW,
        COND_NOT_BELOW,
        COND_EQUAL,
        COND_NOT_EQUAL,
        COND_NOT_ABOVE,
        COND_ABOVE,
        COND_SIGN,
        COND_NOT_SIGN,
        COND_PARITY_EVEN,
        COND_PARITY_ODD,
        COND_LESS,
        COND_NOT_LESS,
        COND_NOT_GREATER,
        COND_GREATER
    };

    static Register REG_NONE;
    static Register RAX;
    static Register RCX;
    static Register RDX;
    static Register RBX;
    static Register RSP;
    static Register RBP;
    static Register RSI;
    static Register RDI;
    static Register R8;
    static Register R9;
    static Register R10;
    static Register R11;
    static Register R12;
    static Register R13;
    static Register R14;
    static Register R15;

    MASM(void *buffer);

    Label label();

    void move64(Register dest, uint64_t immediate);
    void move64(Register dest, Register source);

    void call(Register reg);

    void add32(Register dest, uint32_t imm);
    void add64(Register dest, Register source);

    void ret();

    void compare8(Location where, uint8_t value);
    void compare64(Register a, Register b);

    void load64(Register dest, Location source);
    void store8(Location where, uint8_t value);
    void push64(Register from);
    void pop64(Register to);

    Jump jump32();
    Jump jump32(Condition cond);
    void jumpIndirect(Register where);
    void jumpReallyIndirect(Location where);

    void die();

    void link(Jump jump, Label to);

private:
    static const uint8_t MOD_DEREF = 0;
    static const uint8_t MOD_DEREFPLUS32 = 2;
    static const uint8_t MOD_DIRECT = 3;

    static const uint8_t SS_MULT1 = 0;
    static const uint8_t SS_MULT2 = 1;
    static const uint8_t SS_MULT4 = 2;
    static const uint8_t SS_MULT8 = 3;

    static const uint8_t REG_NO_REX_MAX = 7;

    void *mBase;
    void *mPointer;

    void write8(uint8_t byte);
    void write32(uint32_t dword);
    void write64(uint64_t qword);

    void doModRM(Register r, Register rm);
    void doModRMSIB(Register r, Location rm);
    void doModRM(uint8_t mod, uint8_t r, uint8_t rm);
    void doSIB(uint8_t ss, uint8_t i, uint8_t b);

    void doREX(Register r, Register rm, bool w, bool force = false);
    void doREX(Register r, Location rm, bool w, bool force = false);
    void doREX(uint8_t r, uint8_t x, uint8_t rm, bool rexw, bool force);

};

class MASM::Jump
{
public:
    Jump(unsigned int relTo, unsigned int offset);

    unsigned int getRelativeBase();
    unsigned int getOffsetBase();

private:
    unsigned int mRelativeBase;
    unsigned int mOffsetBase;
};

class MASM::Register
{
public:
    Register(unsigned int number);

    unsigned int getNumber();

private:
    unsigned int mNumber;
};

class MASM::Label
{
public:
    Label(unsigned int offset);

    unsigned int getOffset();

private:
    unsigned int mOffset;
};

class MASM::Location
{
public:
    Location(MASM::Register reg, MASM::Register offsetReg = 0, int multiplier = 0, int offset = 0);

    MASM::Register getBase();
    MASM::Register getOffsetReg();
    int getMultiplier();
    int getOffset();

private:
    MASM::Register mBase;
    MASM::Register mOffsetReg; 
    int mMultiplier;
    int mOffset;
};

#endif
