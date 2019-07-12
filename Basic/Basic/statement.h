/*
 * File: statement.h
 * -----------------
 * This file defines the Statement abstract type.  In
 * the finished version, this file will also specify subclasses
 * for each of the statement types.  As you design your own
 * version of this class, you should pay careful attention to
 * the exp.h interface specified in Chapter 17, which is an
 * excellent model for the Statement class hierarchy.
 */

#ifndef _statement_h
#define _statement_h

#include "evalstate.h"
#include "exp.h"
#include "../StanfordCPPLib/tokenscanner.h"
#include "../StanfordCPPLib/strlib.h"
#include "parser.h"
enum StatementType {LET, PRINT, INPUT, REM , GOTO , IF , END };

/*
 * Class: Statement
 * ----------------
 * This class is used to represent a statement in a program.
 * The model for this class is Expression in the exp.h interface.
 * Like Expression, Statement is an abstract class with subclasses
 * for each of the statement and command types required for the
 * BASIC interpreter.
 */

class Statement {

public:

/*
 * Constructor: Statement
 * ----------------------
 * The base class constructor is empty.  Each subclass must provide
 * its own constructor.
 */

   Statement();

/*
 * Destructor: ~Statement
 * Usage: delete stmt;
 * -------------------
 * The destructor deallocates the storage for this expression.
 * It must be declared virtual to ensure that the correct subclass
 * destructor is called when deleting a statement.
 */

   virtual ~Statement();

/*
 * Method: execute
 * Usage: stmt->execute(state);
 * ----------------------------
 * This method executes a BASIC statement.  Each of the subclasses
 * defines its own execute method that implements the necessary
 * operations.  As was true for the expression evaluator, this
 * method takes an EvalState object for looking up variables or
 * controlling the operation of the interpreter.
 */
   virtual void execute(EvalState & state,int & lineNumber) = 0;
   virtual void execute(EvalState & state) = 0;
   virtual StatementType getType() = 0;
   virtual bool valid()=0;
};

/*
 * The remainder of this file must consists of subclass
 * definitions for the individual statement forms.  Each of
 * those subclasses must define a constructor that parses a
 * statement from a scanner and a method called execute,
 * which executes that statement.  If the private data for
 * a subclass includes data allocated on the heap (such as
 * an Expression object), the class implementation must also
 * specify its own destructor method to free that memory.
 */


/*
 * Seven types of statement are defined below.
 * Each has a member of string to store the original line.
 * Although I hope to parse the line before construst the class, 
 * it confused me when it comes to the two times check for errors.
 * So maybe the string is kind of necessary to simplify the whole program.
 * Besides, I'm not so good at leveraging the abstract class and 
 * the pure virtual function is not as friendly as I expected. I have to  
 * declare another reload of execute function , which is pure virtual, 
 * to enable the control statement to change the linenumber. By the way,
 * the function of valid is for the first check.
 */

class LetStmt: public Statement{  //nothing to say
public:
  LetStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  string exp;
};

class PrintStmt: public Statement{  //nothing to say
public:
  PrintStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  string exp;
};

class InputStmt: public Statement{  //nothing to say
public:
  InputStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  string name,exp;
};

class RemStmt: public Statement{  //nothing to say
public:
  RemStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  string exp;
};

class GotoStmt: public Statement{ //nothing to say
public:
  GotoStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  int number;
  string exp;
};

class IfStmt: public Statement{ //nothing to say
public:
  IfStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  string exp;
};

class EndStmt: public Statement{  //nothing to say
public:
  EndStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber);
  virtual bool valid();
private:
  string exp;
};
/*
class EmptyStmt: public Statement{
public:
  EmptyStmt (string line);
  virtual StatementType getType();
  virtual void execute(EvalState & state);
  virtual void execute(EvalState & state,int & lineNumber); 
  virtual bool valid();
};*/
#endif
