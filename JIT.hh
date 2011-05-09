#ifndef JIT_HH__
#define JIT_HH__

#include "Function.hh"

class JIT
{
public:
    JIT(Function *function, unsigned int *params);

    int run();
    void *compileState(void **stateEntry);
    unsigned char *growTape(unsigned char *idx);
    void debugSpam(int state, unsigned char *idx);

private:
    Function *mFunction;
    unsigned int *mParameters;
    void **mStateArray;
    int mStateCount;
    int mTapeCount;
    int mParameterCount;

    void *mInitialTrampoline;
    void *mCompilerTrampoline;
    void *mGrowTrampoline;

    unsigned char *mTape;
    unsigned int mTapeSize;
    unsigned int mCycle;

    void buildInitialTrampoline();
    void buildCompilerTrampoline();
    void buildGrowTrampoline();
};

#endif
