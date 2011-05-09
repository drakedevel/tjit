#ifndef RULE_HH__
#define RULE_HH__

#include <vector>

#include "Pattern.hh"

class Rule
{
public:
    Rule(int start, int next, std::vector<Pattern *> &cond,
         std::vector<Pattern *> &act, int delta);

    int getFromState();
    int getToState();
    std::vector<Pattern *> *getCondition();
    std::vector<Pattern *> *getAction();
    int getDelta();

private:
    int mFromState;
    int mToState;
    std::vector<Pattern *> mCondition;
    std::vector<Pattern *> mAction;
    int mDelta;

};

#endif
