#include <iostream>

#include "../../include/AST/ASTPrinter.h"
#include "../../include/AST/Expr.h"

using std::cout;
using std::size_t;
using std::string;

using namespace artus;

inline void ASTPrinter::setPiping(unsigned indent) {
  pipingState[indent] = pipingState.find(indent) == pipingState.end() ? \
      true : pipingState[indent];
}

inline void ASTPrinter::clearPiping(unsigned indent) {
  pipingState[indent] = false;
}

inline void ASTPrinter::printPiping() {
  string str = pipeColor;
  for (unsigned idx = 0; idx < indent; idx++) {
    str = pipingState.find(idx) != pipingState.end() && pipingState[idx] ? \
        str + "│ " : str + "  ";
  }
  cout << str;

  if (isLastChild)
    cout << "`-" << clr_clear;
  else
    cout << "|-" << clr_clear;
}

inline void ASTPrinter::printIndent() { cout << string(indent * 2, ' '); }

inline void ASTPrinter::increaseIndent() { indent++; }

inline void ASTPrinter::decreaseIndent() { indent--;}

inline void ASTPrinter::flipLastChild() { isLastChild = !isLastChild; }

inline void ASTPrinter::setLastChild() { isLastChild = 1; }

inline void ASTPrinter::resetLastChild() { isLastChild = 0; }

void ASTPrinter::visit(PackageUnitDecl *decl) {
  cout << unitColor << "PackageUnitDecl " << clr_clear << nameColor << \
      decl->identifier << clr_clear << '\n';
  
  setPiping(indent);
  size_t declCount = decl->decls.size();
  for (unsigned idx = 0; idx < declCount; idx++) {
    if (idx + 1 == declCount) {
      clearPiping(indent);
      setLastChild();
    }

    decl->decls[idx]->pass(this);
    indent = 0;
  }

  resetLastChild();
}

void ASTPrinter::visit(FunctionDecl *decl) {
  printPiping();
  cout << declColor << "FunctionDecl " << clr_clear << nameColor << \
      decl->getName() << clr_clear << typeColor << " '" << decl->T->toString() \
      << "' " << clr_clear << '\n';

  size_t paramsCount = decl->params.size();
  for (unsigned idx = 0; idx < paramsCount; idx++) {
    decl->params[idx]->pass(this);
  }

  setLastChild();
  increaseIndent();
  decl->body->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(ParamVarDecl *decl) {
  cout << "ParamVarDecl" << '\n';
  /*
  cout << declColor << "ParamVarDecl " << clr_clear << nameColor << \
      decl->getName() << clr_clear << qualType << '\'' << decl->T->toString() \
      <<  clr_clear << '\n';
  */
}

void ASTPrinter::visit(LabelDecl *decl) {
  printPiping();
  cout << declColor << "LabelDecl " << clr_clear << nameColor << \
      decl->getName() << clr_clear << '\n';
}

void ASTPrinter::visit(IntegerLiteral *expr) {
  printPiping();
  cout << exprColor << "IntegerLiteral " << clr_clear << typeColor << "'" << \
      expr->T->toString() << "' " << clr_clear << literalColor << \
      expr->getValue() << clr_clear << '\n';
}

void ASTPrinter::visit(CompoundStmt *stmt) {
  printPiping();
  cout << stmtColor << "CompoundStmt " << clr_clear << '\n';

  resetLastChild();
  increaseIndent();
  size_t stmtsCount = stmt->stmts.size();
  for (unsigned idx = 0; idx < stmtsCount; idx++) {
    if (idx + 1 == stmtsCount)
      setLastChild();

    stmt->stmts[idx]->pass(this);
  }

  resetLastChild();
}

void ASTPrinter::visit(LabelStmt *stmt) {
  printPiping();
  cout << stmtColor << "LabelStmt " << clr_clear << nameColor << \
      stmt->getName() << clr_clear << '\n';
}

void ASTPrinter::visit(RetStmt *stmt) {
  printPiping();
  cout << stmtColor << "RetStmt " << clr_clear << '\n';

  setLastChild();
  increaseIndent();
  stmt->expr->pass(this);
  resetLastChild();
}
