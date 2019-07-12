/*
 * File: program.cpp
 * -----------------
 * This file is a stub implementation of the program.h interface
 * in which none of the methods do anything beyond returning a
 * value of the correct type.  Your job is to fill in the bodies
 * of each of these methods with an implementation that satisfies
 * the performance guarantees specified in the assignment.
 */

#include <string>
#include "program.h"
#include "statement.h"
using namespace std;

Program::Program() {

}

Program::~Program() {
    lines.clear();
    stms.clear();
}

void Program::clear(EvalState & state) {    //just clear
    lines.clear();
    stms.clear();
    state.clean();
}

void Program::addSourceLine(int lineNumber, string line) {  //nothing to say
    if (!lines.count(lineNumber)) lines.insert(pair<int,string>(lineNumber,line));
        else lines[lineNumber]=line;
}

void Program::removeSourceLine(int lineNumber) {    //nothing to say
   if (lines.count(lineNumber)) lines.erase(lineNumber);
}

string Program::getSourceLine(int lineNumber) { //nothing to say
    if (lines.count(lineNumber))    return lines[lineNumber];
   return "";    
}

void Program::setParsedStatement(int lineNumber, Statement *stmt) { //nothing to say
    if (!stms.count(lineNumber)) stms.insert(pair<int,Statement*>(lineNumber,stmt));
    else stms[lineNumber]=stmt;
}

Statement *Program::getParsedStatement(int lineNumber) {    //nothing to say
    if (stms.count(lineNumber)) return stms[lineNumber];
   return NULL;  
}

int Program::getFirstLineNumber() { //map has automatic ascending order of the key value,which is cool

    if (lines.size()>0) return lines.begin()->first;
   else return -1;     
}

int Program::getNextLineNumber(int lineNumber) {    //has or hasnt the next linenumber
  if ((++lines.find(lineNumber)!=lines.end())&&(lines.find(lineNumber)!=lines.end()))
            return (++lines.find(lineNumber))->first;
   else return -1;    
}
