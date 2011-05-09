#include <algorithm>
#include <assert.h>
#include <cstring>
#include <err.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>

#include "JIT.hh"
#include "MASM.hh"
#include "xmalloc.h"

using namespace std;

/*
 * RAX: Temporary / Return address
 * RBX: Tape pointer
 * R14: Tape lower bound
 * R15: Tape upper bound
 * RDI: Temporary / Function parameter
 * RSI: Temporary / Function parameter
 * RDX: Temporary / Function parameter
 */

static int log2(unsigned int x)
{
    int result = 0;
    while ((x >>= 1))
        result++;
    return result;
}

static void *NewBuffer()
{
    void *result = mmap(0,
                        4096,
                        PROT_EXEC | PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE,
                        -1,
                        0);
    if (!result)
        err(1, "Unable to allocate JIT buffer");
    return result;
}

// FIXME: Free buffers
//static void DeleteBuffer(void *buffer)
//{
//    munmap(buffer, 4096);
//}

extern "C"
{
static void *CompilerStub(void **stateEntry, JIT *jit)
{
    return jit->compileState(stateEntry);
}
static void *GrowStub(unsigned char *tapePtr, JIT *jit)
{
    return jit->growTape(tapePtr);
}
static void DebugStub(JIT *jit, unsigned int state, unsigned char *tapePtr)
{
    jit->debugSpam(state, tapePtr);
}
}

JIT::JIT(Function *func, unsigned int *params) :
    mFunction(func),
    mParameters(params)
{
}

int JIT::run()
{
    // Figure out max state and max tape
    int maxState = 0;
    int maxTape = mFunction->getArity();
    vector<Rule *> *rules = mFunction->getMachine()->getRules();
    for (vector<Rule *>::iterator i = rules->begin();
         i != rules->end();
         i++)
    {
        maxState = max(maxState, (*i)->getToState());

        vector<Pattern *> *vec = (*i)->getCondition();
        for (vector<Pattern *>::iterator j = vec->begin();
             j != vec->end();
             j++)
        {
            maxTape = max(maxTape, (*j)->getTape());
        }
        vec = (*i)->getAction();
        for (vector<Pattern *>::iterator j = vec->begin();
             j != vec->end();
             j++)
        {
            maxTape = max(maxTape, (*j)->getTape());
        }
    }

    mStateCount = maxState + 1;
    mTapeCount = maxTape + 1;

    // Figure out initial tape length
    // Note that this includes two hashes on the front and back of the value
    int maxParam = 2;
    for (int i = 0; i < mFunction->getArity(); i++)
        maxParam = max(maxParam, log2(mParameters[i]) + 1 + 2);

    // Set up machine state
    mStateArray = new void*[mStateCount];
    mTapeSize = maxParam * mTapeCount;
    mTape = new unsigned char[mTapeSize];

    memset(mTape, 0xffu, mTapeSize);
    for (int i = 0; i < mFunction->getArity(); i++) {
        mTape[i] = 2; // Hash
        int j;
        unsigned int parami = mParameters[i];
        for (j = 1; parami != 0; j++) {
            mTape[mTapeCount * j + i] = parami % 2;
            parami >>= 1;
        }
        mTape[mTapeCount * j + i] = 2; // Hash
    }

    mCycle = 0;

    // Build trampolines
    buildInitialTrampoline();
    buildCompilerTrampoline();
    buildGrowTrampoline();

    // Populate initial state table
    for (int i = 0; i <= maxState; i++)
        mStateArray[i] = mCompilerTrampoline;

    // Jump!
    // FIXME: Hideous
    unsigned char *tapePtr = ((unsigned char *(*)(void *, void *, void *))mInitialTrampoline)(mTape + mTapeCount, mTape, mTape + mTapeSize);

    // Extract our result
    int result = 0;
    int multiplier = 1;
    for (int j = 0; tapePtr[mTapeCount * j + mFunction->getArity()] != 2; j++) {
        result = result + multiplier * tapePtr[mTapeCount * j + mFunction->getArity()];
        multiplier *= 2;
    }

    // Return something useful
    return result;
}

static bool RuleCompare(Rule *l, Rule *r)
{
    return l->getCondition()->size() > r->getCondition()->size();
}

void *JIT::compileState(void **stateEntry)
{
    int state = stateEntry - mStateArray;

    assert(0 <= state && state < mStateCount);
    assert(mStateArray[state] == mCompilerTrampoline);

    printf("Compiling state %d\n", state);

    mStateArray[state] = NewBuffer();
    MASM masm(mStateArray[state]);

    Machine *mach = mFunction->getMachine();

    // Call status updater
    masm.move64(MASM::RDI, (uint64_t)this);
    masm.move64(MASM::RSI, state);
    masm.move64(MASM::RDX, MASM::RBX);
    masm.move64(MASM::RAX, (uint64_t)DebugStub);
    masm.call(MASM::RAX);

    // Check halting state
    if (state == mach->getHaltState()) {
        masm.ret();
        return mStateArray[state];
    }

    // Emit negative tape guard
    masm.compare64(MASM::RBX, MASM::R14);
    MASM::Jump upperPass = masm.jump32(MASM::COND_NOT_LESS);
    masm.move64(MASM::RAX, (uint64_t)mGrowTrampoline);
    masm.call(MASM::RAX);

    // Emit positive tape guard
    masm.link(upperPass, masm.label());
    masm.compare64(MASM::RBX, MASM::R15);
    MASM::Jump lowerPass = masm.jump32(MASM::COND_LESS);
    masm.move64(MASM::RAX, (uint64_t)mGrowTrampoline);
    masm.call(MASM::RAX);
    masm.link(lowerPass, masm.label());

    // Sort by specificity, most specific first
    vector<Rule *> rules;
    for (vector<Rule *>::iterator i = mach->getRules()->begin();
         i != mach->getRules()->end();
         i++)
    {
        if ((*i)->getFromState() == state)
            rules.push_back(*i);
    }    
    sort(rules.begin(), rules.end(), RuleCompare);

    // Emit each rule in turn
    vector<MASM::Jump> nextRuleJumps;
    vector<MASM::Jump> nextStateJumps;
    for (vector<Rule *>::iterator i = rules.begin(); i != rules.end(); i++) {
        Rule *rule = *i;

        // Link previous rules
        for (vector<MASM::Jump>::iterator i = nextRuleJumps.begin();
             i != nextRuleJumps.end();
             i++)
        {
            masm.link(*i, masm.label());
        }
        nextRuleJumps.clear();

        // Emit guards
        vector<Pattern *> *condition = rule->getCondition();
        for (vector<Pattern *>::iterator i = condition->begin();
             i != condition->end();
             i++)
        {
            Pattern *pat = *i;
            masm.compare8(MASM::Location(MASM::RBX,
                                         0,
                                         0,
                                         pat->getTape()),
                          pat->getSymbol());
            MASM::Jump nextRule = masm.jump32(MASM::COND_NOT_EQUAL);
            nextRuleJumps.push_back(nextRule);
        }

        // Emit action
        vector<Pattern *> *action = rule->getAction();
        for (vector<Pattern *>::iterator i = action->begin();
             i != action->end();
             i++)
        {
            Pattern *pat = *i;
            masm.store8(MASM::Location(MASM::RBX,
                                       0,
                                       0,
                                       pat->getTape()),
                        pat->getSymbol());
        }

        // Add tape delta
        masm.add32(MASM::RBX, rule->getDelta() * mTapeCount);

        // Jump to next state
        masm.move64(MASM::RDI, (uint64_t)(&mStateArray[rule->getToState()]));
        masm.jumpReallyIndirect(MASM::Location(MASM::RDI));
    }

    // If we didn't make any matches, die.
    for (vector<MASM::Jump>::iterator i = nextRuleJumps.begin();
         i != nextRuleJumps.end();
         i++)
    {
        masm.link(*i, masm.label());
    }
    masm.die();

    return mStateArray[state];
}

void JIT::buildInitialTrampoline()
{
    Machine *mach = mFunction->getMachine();

    mInitialTrampoline = NewBuffer();
    MASM masm(mInitialTrampoline);

    masm.push64(MASM::RBX);
    masm.push64(MASM::R14);
    masm.push64(MASM::R15);

    masm.move64(MASM::RBX, MASM::RDI);
    masm.move64(MASM::R14, MASM::RSI);
    masm.move64(MASM::R15, MASM::RDX);
    masm.move64(MASM::RDI, (uint64_t)(mStateArray + mach->getInitState()));
    masm.load64(MASM::RAX, MASM::Location(MASM::RDI));
    masm.call(MASM::RAX);
    masm.move64(MASM::RAX, MASM::RBX);

    masm.pop64(MASM::R15);
    masm.pop64(MASM::R14);
    masm.pop64(MASM::RBX);
    masm.ret();
}

void JIT::buildCompilerTrampoline()
{
    mCompilerTrampoline = NewBuffer();
    MASM masm(mCompilerTrampoline);

    // State address in RDI
    masm.move64(MASM::RSI, (uint64_t)this);
    masm.move64(MASM::RAX, (uint64_t)&CompilerStub);
    masm.call(MASM::RAX);
    masm.jumpIndirect(MASM::RAX);
}

void JIT::buildGrowTrampoline()
{
    mGrowTrampoline = NewBuffer();
    MASM masm(mGrowTrampoline);

    // tapePtr = GrowStub(tapePtr)
    masm.move64(MASM::RDI, MASM::RBX); 
    masm.move64(MASM::RSI, (uint64_t)this);
    masm.move64(MASM::RAX, (uint64_t)&GrowStub);
    masm.call(MASM::RAX);
    masm.move64(MASM::RBX, MASM::RAX);

    // lowerBound = mTape
    masm.move64(MASM::R14, (uint64_t)&mTape);
    masm.load64(MASM::R14, MASM::Location(MASM::R14));

    // upperBound = lowerBound + mTapeSize
    masm.move64(MASM::R15, (uint64_t)&mTapeSize);
    masm.load64(MASM::R15, MASM::Location(MASM::R15));
    masm.add64(MASM::R15, MASM::R14);

    masm.ret();
}

unsigned char *JIT::growTape(unsigned char *tapePtr)
{
    int offset = tapePtr - mTape;
    int oldSize = mTapeSize;

    assert(offset >= oldSize);

    printf("Growing tape to size %d.\n", oldSize * 2);

    // Resize tape
    mTapeSize = 2 * oldSize;
    mTape = (unsigned char *) xrealloc(mTape, mTapeSize);

    // Clear uninitialized tape cells
    memset(mTape + oldSize, 0xffu, oldSize);

    return mTape + offset;
}

void JIT::debugSpam(int state, unsigned char *idx)
{
    mCycle++;

    unsigned int index = (idx - mTape) / mTapeCount; 
    bool final = (state == mFunction->getMachine()->getHaltState());
    if (index == 0 || final) {
        printf("--------------------------------------------- Cycle %6d\n", mCycle);
        printf("State %d%s\n", state, final ? " (final)" : "");
        for (int tape = 0; tape < mTapeCount; tape++) {
            printf("Var %3d: ", tape);
            for (unsigned int cell = 0; cell < mTapeSize / mTapeCount; cell++) {
                char c;
                switch (mTape[cell * mTapeCount + tape]) {
                case 0:
                    c = '0';
                    break;
                case 1:
                    c = '1';
                    break;
                case 2:
                    c = '#';
                    break;
                default:
                    c = ' ';
                    break;
                }
                printf("%c ", c); 
            }
            printf("\n");
        }
    }
}
