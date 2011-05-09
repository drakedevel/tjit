#include "Parser.hh"

#include <assert.h>
#include <cstdlib>
#include <err.h>

using namespace std;

Parser::Parser(const string &input) : mInput(input)
{
}

map<string, Function *> *Parser::parse()
{
    unsigned int idx = 0;
    SExpr *sexpr = parseSexpr(idx);
    if (!sexpr)
        return 0;

    map<string, Function *> *result = new map<string, Function *>();
    for (int i = 0; i < sexpr->length(); i++) {
        if (!sexpr->isSexpr(i))
            return 0;
        Function *function = parseFunction(sexpr->getSexpr(i));
        if (!function)
            return 0;
        (*result)[*function->getName()] = function;
    }

    return result;
}

Function *Parser::parseFunction(SExpr *sexpr)
{
    if (!sexpr->isString(0) || !sexpr->isString(1) || !sexpr->isSexpr(2))
        return 0;
    
    string *name = sexpr->getString(0);
    string *arityStr = sexpr->getString(1);
    int arity = strtol(arityStr->c_str(), NULL, 0);
    Machine *machine = parseMachine(sexpr->getSexpr(2));
    if (!machine)
        return 0;

    return new Function(*name, arity, machine);
}

Machine *Parser::parseMachine(SExpr *sexpr)
{
    if (!sexpr->isString(0) ||
        !sexpr->isString(1) ||
        !sexpr->isSexpr(2)) {
        assert(0);
        return 0;
    }

    string *startStr = sexpr->getString(0);
    string *haltStr = sexpr->getString(1);
    vector<Rule *> *rules = parseRules(sexpr->getSexpr(2));

    int start = strtol(startStr->c_str(), NULL, 0);
    int halt = strtol(haltStr->c_str(), NULL, 0);
    assert(rules);
    if (!rules)
        return 0;

    return new Machine(start, halt, *rules);
}

vector<Rule *> *Parser::parseRules(SExpr *sexpr)
{
    vector<Rule *> *result = new vector<Rule *>();
    for (int i = 0; i < sexpr->length(); i++) {
        if (!sexpr->isSexpr(i))
            return 0;
        Rule *rule = parseRule(sexpr->getSexpr(i));
        if (!rule)
            return 0;
        result->push_back(rule);
    }
    return result;
}

Rule *Parser::parseRule(SExpr *sexpr)
{
    if (!sexpr->isString(0) ||
        !sexpr->isString(1) ||
        !sexpr->isSexpr(2) ||
        !sexpr->isSexpr(3) ||
        !sexpr->isString(4)) {
        return 0;
    }

    string *startStr = sexpr->getString(0);
    string *nextStr = sexpr->getString(1);
    vector<Pattern *> *condPatterns = parsePatterns(sexpr->getSexpr(2));
    vector<Pattern *> *actPatterns = parsePatterns(sexpr->getSexpr(3));
    string *deltaStr = sexpr->getString(4);

    int start = strtol(startStr->c_str(), NULL, 0);
    int next = strtol(nextStr->c_str(), NULL, 0);
    if (!condPatterns || !actPatterns)
        return 0;
    int delta = strtol(deltaStr->c_str(), NULL, 0);

    return new Rule(start, next, *condPatterns, *actPatterns, delta);
}

std::vector<Pattern *> *Parser::parsePatterns(SExpr *sexpr)
{
    vector<Pattern *> *result = new vector<Pattern *>();
    for (int i = 0; i < sexpr->length(); i++) {
        if (!sexpr->isSexpr(i))
            return 0;
        Pattern *rule = parsePattern(sexpr->getSexpr(i));
        if (!rule)
            return 0;
        result->push_back(rule);
    }
    return result;
}

Pattern *Parser::parsePattern(SExpr *sexpr)
{
    if (sexpr->length() < 2 || !sexpr->isString(0) || !sexpr->isString(1))
        return 0;

    string *symbolStr = sexpr->getString(0);
    string *tapeStr = sexpr->getString(1);

    unsigned char symbol = (unsigned char)strtoul(symbolStr->c_str(), NULL, 0);
    unsigned int tape = strtoul(tapeStr->c_str(), NULL, 0);

    return new Pattern(symbol, tape);
}

Parser::SExpr *Parser::parseSexpr(unsigned int &idx)
{
    assert(idx < mInput.length());
    assert(mInput[idx] == '(');

    SExpr *result = new SExpr();

    idx++;
    while (idx < mInput.length()) {
        switch (mInput[idx]) {
        case ' ':
            idx++;
            break;

        case '(': {
            SExpr *sub = parseSexpr(idx);
            if (!sub)
                return 0;
            result->push(sub);
            break;
        }

        case ')':
            idx++;
            return result;

        default: {
            unsigned int sidx = idx + 1;
            while (sidx < mInput.length() && mInput[sidx] != ' ' && mInput[sidx] != ')')
                sidx++;

            if (sidx == mInput.length())
                return 0;

            result->push(mInput.substr(idx, sidx - idx));
            idx = sidx;
            break;
        }

        }
    }

    return 0;
}

void Parser::SExpr::push(SExpr *sexpr)
{
    mValues.push_back(sexpr);
}

void Parser::SExpr::push(const std::string &str)
{
    mValues.push_back(new string(str));
}

int Parser::SExpr::length()
{
    return mValues.size();
}

bool Parser::SExpr::isSexpr(unsigned int index)
{
    return mValues[index].isSexpr();
}

bool Parser::SExpr::isString(unsigned int index)
{
    return mValues[index].isString();
}

Parser::SExpr *Parser::SExpr::getSexpr(unsigned int index)
{
    return mValues[index].getSexpr();
}

std::string *Parser::SExpr::getString(unsigned int index)
{
    return mValues[index].getString();
}

Parser::SExpr::SExprOrString::SExprOrString(SExpr *sexpr) : mIsSexpr(true)
{
    mValue.sexpr = sexpr;
}

Parser::SExpr::SExprOrString::SExprOrString(string *str) : mIsSexpr(false)
{
    mValue.str = str;
}

bool Parser::SExpr::SExprOrString::isSexpr() const
{
    return mIsSexpr;
}

bool Parser::SExpr::SExprOrString::isString() const
{
    return !mIsSexpr;
}

Parser::SExpr *Parser::SExpr::SExprOrString::getSexpr() const
{
    assert(mIsSexpr);
    return mValue.sexpr;
}

string *Parser::SExpr::SExprOrString::getString() const
{
    assert(!mIsSexpr);
    return mValue.str;
}

