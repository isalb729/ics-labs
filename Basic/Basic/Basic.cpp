/*
 * File: Basic.cpp
 * ---------------
 * Name: [Yu Yajie]
 * Section: [Yu Yajie]
 * This file is the starter project for the BASIC interpreter from
 * Assignment #6.
 */
#include <cctype>
#include <iostream>
#include <string>
#include "exp.h"
#include "parser.h"
#include "program.h"
#include "../StanfordCPPLib/error.h"
#include "../StanfordCPPLib/tokenscanner.h"
#include "../StanfordCPPLib/simpio.h"
#include "../StanfordCPPLib/strlib.h"
using namespace std;

/* Function prototypes */

void processLine(string line, Program & program, EvalState & state);

/* Main program */

int main() {
   EvalState state;
   Program program;
  // cout << "Stub implementation of BASIC" << endl;
   while (true) {
      try {
         processLine(getLine(), program, state);    //processing the every input line
      } catch (ErrorException & ex) {
         cout << ex.getMessage() << endl;   //show the five types of errors
      }
   }
    return 0;
}

/*
 * Function: processLine
 * Usage: processLine(line, program, state);
 * -----------------------------------------
 * Processes a single line entered by the user.  In this version,
 * the implementation does exactly what the interpreter program
 * does in Chapter 19: read a line, parse it as an expression,
 * and then print the result.In your implementation, you will
 * need to replace this method with one that can respond correctly
 * when the user enters a program line (which begins with a number)
 * or one of the BASIC commands, such as LIST or RUN.
 */

void processLine(string line, Program & program, EvalState & state) {
   
   TokenScanner scanner;
   scanner.ignoreWhitespace();
   scanner.scanNumbers();
   scanner.setInput(line);
   if (!scanner.hasMoreTokens()) return ;   //NUll line should be ignored
   string token=scanner.nextToken();
   TokenType tpye=scanner.getTokenType(token);
   /*store the linenumber and the respective statement*/
    if (tpye==NUMBER) {
        int lineNumber=stringToInteger(token);       
        program.addSourceLine(lineNumber,line);
        if (!scanner.hasMoreTokens()) { //NULL statement are supposed to remove the line
        program.removeSourceLine(lineNumber);
        return ;
        }
        token=scanner.nextToken();
        /*
         * seven types of statement shares the similar way to store
         * substring is supposed to omit the statement type  
         * the first check of validation of the statement is about the judgement of the form   
         * then store the statement and corresponding linenumber
        */
        if (token=="LET") {
        line=line.substr(line.find("LET")+3);
        Statement* stmt=new LetStmt(line);
        if (stmt->valid()) program.setParsedStatement(lineNumber,stmt);
        return ;
        }
        if (token=="PRINT") {
        line=line.substr(line.find("PRINT")+5);
        Statement* stmt=new PrintStmt(line);
        if (stmt->valid())  program.setParsedStatement(lineNumber,stmt);
        return ;
        }
        if (token=="INPUT") {
        line=line.substr(line.find("INPUT")+5);
        Statement* stmt=new InputStmt(line);
        if (stmt->valid()) program.setParsedStatement(lineNumber,stmt);
        return ;
        }
        if (token=="REM") {
        line=line.substr(line.find("REM")+3);
        Statement* stmt=new RemStmt(line);
        if (stmt->valid()) program.setParsedStatement(lineNumber,stmt);
        return ;
        }
        if (token=="GOTO") {
        line=line.substr(line.find("GOTO")+4);
        Statement* stmt=new GotoStmt(line);
        if (stmt->valid()) program.setParsedStatement(lineNumber,stmt);
        return ;
        }
        if (token=="IF") {
        line=line.substr(line.find("IF")+2);
        Statement* stmt=new IfStmt(line);
        if (stmt->valid()) program.setParsedStatement(lineNumber,stmt);
        return ;
        }
        if (token=="END") {
        line=line.substr(line.find("END")+3);
        Statement* stmt=new EndStmt(line);
        if (stmt->valid()) program.setParsedStatement(lineNumber,stmt);
        return ;
         }
        error("SYNTAX ERROR");  //if token is not a valid statement type
    }
    /*
     * 8 types of commands are processed distinctively
     * 3 types of control statement are processed in the same way as shown above
     */
    if (token=="RUN"){
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR"); 
        int lineNumber=program.getFirstLineNumber();
        while (lineNumber!=-1) { //which means the program has run to the end
            StatementType type=program.getParsedStatement(lineNumber)->getType();
            if ((type==END)||(type==GOTO)||(type==IF)) {    //control statement is enabled to change the current linenumber
                int copy=lineNumber;    //to judge whether the condition of  if statement is false  
                program.getParsedStatement(lineNumber)->execute(state,lineNumber);
                if (lineNumber==copy) lineNumber=program.getNextLineNumber(lineNumber);
                }
            else {program.getParsedStatement(lineNumber)->execute(state);   //just execute and go on the program
            lineNumber=program.getNextLineNumber(lineNumber);
            }
            if ((program.getParsedStatement(lineNumber)==NULL)&&(lineNumber!=-1)) error("LINE NUMBER ERROR");   //wrong linenumber is given
        }
        return ;
    }
    if (token=="LIST"){ //just list
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        int lineNumber=program.getFirstLineNumber();
        while (lineNumber!=-1)
        {
            cout<<program.getSourceLine(lineNumber)<<endl;
            lineNumber=program.getNextLineNumber(lineNumber);
        }
        return;
    }
    if (token=="HELP"){ //just help
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        cout<<"Yet another basic interpreter"<<endl;
        return;
    }
    if (token=="CLEAR"){    //just clear the state and the lines
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        program.clear(state);
        return;
    }
    if (token=="QUIT"){ //just quit
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        exit(0);
    }
    if (token=="LET"){  //just let var=value
        line=line.substr(line.find("LET")+3);
        Statement* stmt=new LetStmt(line);
        if (stmt->valid()) stmt->execute(state);
        return;
    }
     if (token=="INPUT"){   //just input
        line=line.substr(line.find("INPUT")+5);
        Statement* stmt=new InputStmt(line);
        if (stmt->valid()) stmt->execute(state);
        return;
    }
     if (token=="PRINT"){   //just print
        line=line.substr(line.find("PRINT")+5);
        Statement* stmt=new PrintStmt(line);
        if (stmt->valid()) stmt->execute(state);
        return;
    }
    error("SYNTAX ERROR");  //unsolvable tokentype
  
}