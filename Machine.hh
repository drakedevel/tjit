#ifndef MACHINE_HH__
#define MACHINE_HH__

#include <vector>

#include "Rule.hh"

class Machine
{
public:
    Machine(int start, int halt, std::vector<Rule *> &rules);

    int getInitState();
    int getHaltState();
    std::vector<Rule *> *getRules();

private:
    int mInitState;
    int mHaltState;
    std::vector<Rule *> mRules;

};

#endif
