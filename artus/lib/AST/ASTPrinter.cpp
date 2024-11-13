#include <iostream>
#include <cstring>

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

string ASTPrinter::spanToString(const Span &span) {
  return '<' + spanColor + span.file + clear + "<" + spanColor + \
         std::to_string(span.line) + ':' + std::to_string(span.col) + \
         clear + ", " + spanColor + std::to_string(span.line_nd) + ':' + \
         std::to_string(span.col_nd) + clear + ">>";
}

void ASTPrinter::printDecl(const string &node, const string &name,
                           const string &type, bool newl) {
  cout << declColor << node << clear << string(name.empty() ? 0 : 1, ' ') \
       << nameColor << name << clear;
       
  if (!type.empty())
    cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    cout << '\n';
}

void ASTPrinter::printDecl(const Span &span, const string &node, 
                           const string &name, const string &type, bool newl) {
  cout << declColor << node << clear << ' ' << spanToString(span) \
       << string(name.empty() ? 0 : 1, ' ') << nameColor << name << clear;

  if (!type.empty())
    cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    cout << '\n';                   
}

void ASTPrinter::printExpr(const Span &span, const string &node, 
                           const string &type, const string &ident, bool newl) {
  cout << exprColor << node << clear << ' ' << spanToString(span) \
       << string(ident.empty() ? 0 : 1, ' ') << nameColor << ident << clear;

  if (!type.empty())
    cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    cout << '\n';
}

void ASTPrinter::printStmt(const Span &span, const string &node, 
                           const string &name, bool newl) {
  cout << stmtColor << node << clear << ' ' << spanToString(span) \
       << string(name.empty() ? 0 : 1, ' ') << nameColor << name \
       << clear;

  if (newl) 
    cout << '\n';
}

void ASTPrinter::visit(PackageUnitDecl *decl) {
  printDecl("PackageUnitDecl", decl->identifier, "");
  
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
  printDecl(decl->span, "FunctionDecl", decl->name, decl->T->toString());

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
  printPiping();
  printDecl(decl->span, "ParamVarDecl", decl->name, decl->T->toString());
}

void ASTPrinter::visit(LabelDecl *decl) { /* unused */ }

void ASTPrinter::visit(VarDecl *decl) {
  printPiping();
  printDecl(decl->span, "VarDecl", decl->name, decl->T->toString(), false);

  if (decl->mut) {
    cout << " mut";
  }
  cout << '\n';

  setLastChild();
  increaseIndent();
  decl->init->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(ImplicitCastExpr *expr) {
  printPiping();
  printExpr(expr->span, "ImplicitCastExpr", expr->T->toString());

  setLastChild();
  increaseIndent();
  expr->expr->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(ExplicitCastExpr *expr)  {
  printPiping();
  printExpr(expr->span, "ExplicitCastExpr", expr->T->toString());

  setLastChild();
  increaseIndent();
  expr->expr->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(DeclRefExpr *expr) {
  printPiping();
  printExpr(expr->span, "DeclRefExpr", expr->T->toString(), expr->ident);
}

void ASTPrinter::visit(BinaryExpr *expr) {
  printPiping();
  printExpr(expr->span, "BinaryExpr", expr->T->toString(), "", false);

  //cout << literalColor << expr->op << clear << '\n';
  cout << '\n';

  resetLastChild();
  increaseIndent();
  expr->lhs->pass(this);
  setLastChild();
  expr->rhs->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(IntegerLiteral *expr) {
  printPiping();
  printExpr(expr->span, "IntegerLiteral", expr->T->toString(), "", false);
  cout << ' ' << literalColor << expr->value << clear << '\n';
}

void ASTPrinter::visit(CompoundStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "CompoundStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  for (unsigned idx = 0; idx < stmt->stmts.size(); idx++) {
    if (idx + 1 == stmt->stmts.size()) {
      clearPiping(indent);
      setLastChild();
    }

    stmt->stmts[idx]->pass(this);
  }

  resetLastChild();
}

void ASTPrinter::visit(DeclStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "DeclStmt");
  setLastChild();
  increaseIndent();
  stmt->decl->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(LabelStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "LabelStmt", stmt->name);
}

void ASTPrinter::visit(JmpStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "JmpStmt", stmt->name);
}

void ASTPrinter::visit(RetStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "RetStmt");
  setLastChild();
  increaseIndent();
  stmt->expr->pass(this);
  resetLastChild();
}
