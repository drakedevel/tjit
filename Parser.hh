#ifndef PARSER_HH__
#define PARSER_HH__

#include <map>
#include <string>
#include <vector>

#include "Function.hh"

class Parser
{
public:
    Parser(const std::string &input);

    std::map<std::string, Function *> *parse();

private:
    class SExpr;

    std::string mInput;

    Function *parseFunction(SExpr *sexpr);

    Machine *parseMachine(SExpr *sexpr);

    std::vector<Rule *> *parseRules(SExpr *sexpr);

    Rule *parseRule(SExpr *sexpr);

    std::vector<Pattern *> *parsePatterns(SExpr *sexpr);

    Pattern *parsePattern(SExpr *sexpr);

    SExpr *parseSexpr(unsigned int& idx);
};

class Parser::SExpr
{
public:
    void push(SExpr *sexpr);

    void push(const std::string &str);

    int length();

    bool isSexpr(unsigned int index);

    bool isString(unsigned int index);

    SExpr *getSexpr(unsigned int index);

    std::string *getString(unsigned int index);

private:
    class SExprOrString;

    std::vector<SExprOrString> mValues;

};

class Parser::SExpr::SExprOrString
{
public:
    SExprOrString(SExpr *sexpr);
    
    SExprOrString(std::string *string);
    
    bool isSexpr() const;
    
    bool isString() const;
    
    SExpr *getSexpr() const;
    
    std::string *getString() const;

private:
    bool mIsSexpr;
    union
    {
        SExpr *sexpr;
        std::string *str;
    } mValue;
};

#endif
