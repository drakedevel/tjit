#include "Machine.hh"

using namespace std;

Machine::Machine(int init, int halt, vector<Rule *> &rules) :
    mInitState(init),
    mHaltState(halt),
    mRules(rules)
{

}

int Machine::getInitState()
{
    return mInitState;
}

int Machine::getHaltState()
{
    return mHaltState;
}

vector<Rule *> *Machine::getRules()
{
    return &mRules;
}
