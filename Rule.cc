#include "Rule.hh"

using namespace std;

Rule::Rule(int from, int to, vector<Pattern *> &condition, vector<Pattern *> &action, int delta) :
    mFromState(from),
    mToState(to),
    mCondition(condition),
    mAction(action),
    mDelta(delta)
{
}

int Rule::getFromState()
{
    return mFromState;
}

int Rule::getToState()
{
    return mToState;
}

vector<Pattern *> *Rule::getCondition()
{
    return &mCondition;
}

vector<Pattern *> *Rule::getAction()
{
    return &mAction;
}

int Rule::getDelta()
{
    return mDelta;
}
