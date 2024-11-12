#include <iostream>

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
    cout << "`-" << clear;
  else
    cout << "|-" << clear;
}

inline void ASTPrinter::printIndent() { cout << string(indent * 2, ' '); }

inline void ASTPrinter::increaseIndent() { indent++; }

inline void ASTPrinter::decreaseIndent() { indent--;}

inline void ASTPrinter::flipLastChild() { isLastChild = !isLastChild; }

inline void ASTPrinter::setLastChild() { isLastChild = 1; }

inline void ASTPrinter::resetLastChild() { isLastChild = 0; }

void ASTPrinter::visit(PackageUnitDecl *decl) {
  cout << unitColor << "PackageUnitDecl " << clear << nameColor << \
      decl->identifier << clear << '\n';
  
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
  cout << declColor << "FunctionDecl " << clear << nameColor << \
      decl->getName() << clear << typeColor << " '" << decl->T->toString() \
      << "' " << clear << '\n';

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
  cout << declColor << "ParamVarDecl " << clear << nameColor << \
      decl->getName() << clear << typeColor << '\'' << decl->T->toString() \
      <<  clear << '\n';
}

void ASTPrinter::visit(LabelDecl *decl) {
  printPiping();
  cout << declColor << "LabelDecl " << clear << nameColor << \
      decl->getName() << clear << '\n';
}

void ASTPrinter::visit(ImplicitCastExpr *expr) {
  printPiping();
  cout << exprColor << "ImplicitCastExpr " << clear << typeColor << "'" << \
      expr->T->toString() << "' " << clear << '\n';

  setLastChild();
  increaseIndent();
  expr->expr->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(ExplicitCastExpr *expr)  {
  printPiping();
  cout << exprColor << "ExplicitCastExpr " << clear << typeColor << "'" << \
      expr->T->toString() << "' " << clear << '\n';

  setLastChild();
  increaseIndent();
  expr->expr->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(IntegerLiteral *expr) {
  printPiping();
  cout << exprColor << "IntegerLiteral " << clear << typeColor << "'" << \
      expr->T->toString() << "' " << clear << literalColor << \
      expr->value << clear << '\n';
}

void ASTPrinter::visit(CompoundStmt *stmt) {
  printPiping();
  cout << stmtColor << "CompoundStmt " << clear << '\n';

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
  cout << stmtColor << "LabelStmt " << clear << nameColor << \
      stmt->getName() << clear << '\n';
}

void ASTPrinter::visit(RetStmt *stmt) {
  printPiping();
  cout << stmtColor << "RetStmt " << clear << '\n';

  setLastChild();
  increaseIndent();
  stmt->expr->pass(this);
  resetLastChild();
}
