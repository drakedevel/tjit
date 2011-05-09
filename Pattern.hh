#ifndef PATTERN_HH__
#define PATTERN_HH__

class Pattern
{
public:
    Pattern(unsigned char symbol, unsigned int tape);

    unsigned char getSymbol();
    int getTape();

private:
    unsigned char mSymbol;
    unsigned int mTape;

};

#endif
