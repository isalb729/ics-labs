/*
 * File: statement.cpp
 * -------------------
 * This file implements the constructor and destructor for
 * the Statement class itself.  Your implementation must do
 * the same for the subclasses you define for each of the
 * BASIC statements.
 */

#include <string>
#include "statement.h"
#include <sstream>
using namespace std;

/* Implementation of the Statement class */

Statement::Statement() {
   /* Empty */
}

Statement::~Statement() {
   /* Empty */
}

LetStmt::LetStmt(string line):exp(line){

}
StatementType LetStmt::getType(){
    return LET;
}

void LetStmt::execute(EvalState & state){
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    string name=scanner.nextToken();    //get the var name
    scanner.nextToken();
    Expression *e=parseExp(scanner);
    int value=e->eval(state);   //get the value
    state.setValue(name,value); //set it
}

void LetStmt::execute(EvalState & state,int & lineNumber){

}

bool LetStmt::valid(){
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");    //basic operation
    string name=scanner.nextToken();    
    if ((scanner.getTokenType(name)!=WORD)||(name=="LET")||(name=="RUN")||(name=="INPUT")||(name=="QUIT")||(name=="HELP")||(name=="LIST")||(name=="IF")||(name=="THEN")||(name=="GOTO")||(name=="REM")) error("SYNTAX ERROR"); 
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    if (scanner.nextToken()!="=") error("SYNTAX ERROR");
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    Expression *e=parseExp(scanner);    //get the expression
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    return true;
}

PrintStmt::PrintStmt(string line):exp(line){
    
}

StatementType PrintStmt::getType(){
    return PRINT;
}

void PrintStmt::execute(EvalState & state){
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    Expression *ex = parseExp(scanner);
    cout<<ex->eval(state)<<endl;    //just cout
}

void PrintStmt::execute(EvalState & state,int & lineNumber){
    
}

bool PrintStmt::valid(){    //basic operation
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    Expression *ex = parseExp(scanner);
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    return true;
}

InputStmt::InputStmt(string line):exp(line){
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(line);
    name=scanner.nextToken();   //basic operation
}

StatementType InputStmt::getType(){
    return INPUT;
}

void InputStmt::execute(EvalState & state){
    string s,vstr;
    int sign;
    while (true){   //seems to circulate if wrong inputs are given
    cout<<" ? ";
    sign=1;
    getline(cin,s);
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(s);
    vstr=scanner.nextToken();
    if (vstr=="-") {sign=-1;vstr=scanner.nextToken();}  //negative values should be handled first
    if ((vstr.find(".")!=string::npos)||(scanner.getTokenType(vstr)!=NUMBER)||(scanner.hasMoreTokens())){cout<<("INVALID NUMBER")<<endl;continue;}
    //I found the token will accept real numbers.It's not cool.
    break;
    }
    int value=stringToInteger(vstr)*sign;
    state.setValue(name,value);
}

void InputStmt::execute(EvalState & state,int & lineNumber){
    
}

bool InputStmt::valid(){
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    name=scanner.nextToken();
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR"); //basic operation
    return true;
}

RemStmt::RemStmt(string line):exp(line){

}

StatementType RemStmt::getType(){
    return REM;
}

void RemStmt::execute(EvalState & state){   //nothing to say

}

void RemStmt::execute(EvalState & state,int & lineNumber){ //nothing to say
    
}

bool RemStmt::valid(){ //nothing to say
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    return true;
}

GotoStmt::GotoStmt(string line):exp(line){

}

StatementType GotoStmt::getType(){
    return GOTO;
}

void GotoStmt::execute(EvalState & state){

}

void GotoStmt::execute(EvalState & state,int & lineNumber){ //just change
    lineNumber=number;
}

bool GotoStmt::valid(){ //nothing to say
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    number=stringToInteger(scanner.nextToken());
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    return true;    
}
IfStmt::IfStmt(string line):exp(line){

}

StatementType IfStmt::getType(){
    return IF;
}

void IfStmt::execute(EvalState & state){

}

void IfStmt::execute(EvalState & state,int & lineNumber){   //easy to understand,change the lineNumber
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    Expression *lhs = parseExp(scanner);
    int lvalue = lhs->eval(state);
    string op=scanner.nextToken();
    Expression *rhs = parseExp(scanner);
    int rvalue = rhs->eval(state);
    scanner.nextToken();
    int number=stringToInteger(scanner.nextToken());
    if (((op=="=")&&(lvalue==rvalue))||((op==">")&&(lvalue>rvalue))||((op=="<")&&(lvalue<rvalue))) lineNumber=number;
    delete lhs;
    delete rhs;
}

bool IfStmt::valid(){   //get the every token and judege whether they match the standard form
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    Expression *lhs = parseExp(scanner);
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    if (scanner.getTokenType(scanner.nextToken())!=OPERATOR) error("SYNTAX ERROR");
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    Expression *rhs = parseExp(scanner);
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    if (scanner.nextToken()!="THEN") error("SYNTAX ERROR");
    if (!scanner.hasMoreTokens()) error("SYNTAX ERROR");
    string s=scanner.nextToken();
    if (scanner.getTokenType(s)!=NUMBER) error("SYNTAX ERROR");
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    delete lhs;
    delete rhs;
    return true;
}

EndStmt::EndStmt(string line):exp(line){

}

StatementType EndStmt::getType(){
    return END;
}

void EndStmt::execute(EvalState & state){

}

void EndStmt::execute(EvalState & state,int & lineNumber){
    lineNumber=-1;
}

bool EndStmt::valid(){  //end the program
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(exp);
    if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    return true;
}
/*
 *  At first I consider that Null statement is also supposed
 *  to be stored even though it's of no value.
 */
/*EmptyStmt::EmptyStmt(string line){

}

StatementType EmptyStmt::getType(){
    return EMPTY;
}

void EmptyStmt::execute(EvalState & state){

}

void EmptyStmt::execute(EvalState & state,int & lineNumber){
}

bool EmptyStmt::valid(){
    return true;
}*/