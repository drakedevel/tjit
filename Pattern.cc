#include "Pattern.hh"

Pattern::Pattern(unsigned char symbol, unsigned int tape) :
    mSymbol(symbol),
    mTape(tape)
{
}

unsigned char Pattern::getSymbol()
{
    return mSymbol;
}

int Pattern::getTape()
{
    return mTape;
}
