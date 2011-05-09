#ifndef FUNCTION_HH__
#define FUNCTION_HH__

#include <string>

#include "Machine.hh"

class Function
{
public:
    Function(const std::string &name, int arity, Machine *machine);

    std::string *getName();

    int getArity();

    Machine *getMachine();

private:
    std::string mName;
    int mArity;
    Machine *mMachine;

};

#endif
