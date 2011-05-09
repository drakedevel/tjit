#include "Function.hh"

using namespace std;

Function::Function(const string &name, int arity, Machine *machine) :
    mName(name),
    mArity(arity),
    mMachine(machine)
{
}

string *Function::getName()
{
    return &mName;
}

int Function::getArity()
{
    return mArity;
}

Machine *Function::getMachine()
{
    return mMachine;
}
