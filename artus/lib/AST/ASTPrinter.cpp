#include <iostream>
#include <cstring>

#include "../../include/AST/ASTPrinter.h"
#include "../../include/AST/Expr.h"
#include "Decl.h"

using std::cout;
using std::size_t;
using std::string;

using namespace artus;

/// Returns a string representation of a unary operator.
inline static string unaryOpToString(UnaryExpr::UnaryOp op) {
  switch (op) {
    case UnaryExpr::UnaryOp::Negative: 
      return "-";
    case UnaryExpr::UnaryOp::Not: 
      return "!";
    case UnaryExpr::UnaryOp::Ref: 
      return "&";
    case UnaryExpr::UnaryOp::DeRef: 
      return "#";
    default: return " ";
  }
}

/// Returns a string representation of a binary operator.
inline static string binaryOpToString(BinaryExpr::BinaryOp op) {
  switch (op) {
    case BinaryExpr::BinaryOp::Assign: 
      return "=";
    case BinaryExpr::BinaryOp::AddAssign: 
      return "+=";
    case BinaryExpr::BinaryOp::SubAssign: 
      return "-=";
    case BinaryExpr::BinaryOp::MultAssign: 
      return "*=";
    case BinaryExpr::BinaryOp::DivAssign: 
      return "/=";
    case BinaryExpr::BinaryOp::Equals: 
      return "==";
    case BinaryExpr::BinaryOp::NotEquals: 
      return "!=";
    case BinaryExpr::BinaryOp::LessThan: 
      return "<";
    case BinaryExpr::BinaryOp::LessEquals: 
      return "<=";
    case BinaryExpr::BinaryOp::GreaterThan: 
      return ">";
    case BinaryExpr::BinaryOp::GreaterEquals: 
      return ">=";
    case BinaryExpr::BinaryOp::LogicalAnd: 
      return "&&";
    case BinaryExpr::BinaryOp::LogicalOr: 
      return "||";
    case BinaryExpr::BinaryOp::LogicalXor: 
      return "^^";
    case BinaryExpr::BinaryOp::Add: 
     return "+";
    case BinaryExpr::BinaryOp::Sub: 
      return "-";
    case BinaryExpr::BinaryOp::Mult: 
      return "*";
    case BinaryExpr::BinaryOp::Div: 
      return "/";
    default: 
      return " ";
  }
}

inline void ASTPrinter::setPiping(unsigned indent) {
  pipingState[indent] = pipingState.find(indent) == pipingState.end() ? true 
      : pipingState[indent];
}

inline void ASTPrinter::clearPiping(unsigned indent) {
  pipingState[indent] = false;
}

inline void ASTPrinter::printPiping() {
  string str = pipeColor;
  for (unsigned idx = 0; idx < indent; idx++) {
    str = pipingState.find(idx) != pipingState.end() && pipingState[idx]
          ? str + "│ " : str + "  ";
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
  return '<' + spanColor + span.file + clear + "<" + spanColor 
         + std::to_string(span.line) + ':' + std::to_string(span.col) 
         + clear + ", " + spanColor + std::to_string(span.line_nd) + ':' 
         + std::to_string(span.col_nd) + clear + ">>";
}

void ASTPrinter::printDecl(const string &node, const string &name,
                           const string &type, bool newl) {
  cout << declColor << node << clear << string(name.empty() ? 0 : 1, ' ')
       << nameColor << name << clear;
       
  if (!type.empty())
    cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    cout << '\n';
}

void ASTPrinter::printDecl(const Span &span, const string &node, 
                           const string &name, const string &type, bool newl) {
  cout << declColor << node << clear << ' ' << spanToString(span)
       << string(name.empty() ? 0 : 1, ' ') << nameColor << name << clear;

  if (!type.empty())
    cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    cout << '\n';                   
}

void ASTPrinter::printExpr(const Span &span, const string &node, 
                           const string &type, const string &ident, bool newl) {
  cout << exprColor << node << clear << ' ' << spanToString(span)
       << string(ident.empty() ? 0 : 1, ' ') << nameColor << ident << clear;

  if (!type.empty())
    cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    cout << '\n';
}

void ASTPrinter::printStmt(const Span &span, const string &node, 
                           const string &name, bool newl) {
  cout << stmtColor << node << clear << ' ' << spanToString(span)
       << string(name.empty() ? 0 : 1, ' ') << nameColor << name
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

void ASTPrinter::visit(ImportDecl *decl) {
  printPiping();
  printDecl(decl->span, "ImportDecl", decl->path.curr);
}

void ASTPrinter::visit(FunctionDecl *decl) {
  printPiping();
  printDecl(decl->span, "FunctionDecl", decl->name, 
      decl->T->toString(), false);

  if (decl->isPrivate()) {
    cout << " private\n";
  } else {
    cout << '\n';
  }

  increaseIndent();
  size_t paramsCount = decl->params.size();
  for (unsigned idx = 0; idx < paramsCount; idx++) {
    decl->params[idx]->pass(this);
  }

  setLastChild();
  decl->body->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(ParamVarDecl *decl) {
  printPiping();
  printDecl(decl->span, "ParamVarDecl", decl->name, decl->T->toString());
}

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

void ASTPrinter::visit(EnumDecl *decl) {
  printPiping();
  printDecl(decl->span, "EnumDecl", decl->name, "", false);

  if (decl->isPrivate()) {
    cout << " private\n";
  } else {
    cout << '\n';
  }

  increaseIndent();
  setPiping(indent);
  size_t variantsCount = decl->variants.size();
  for (unsigned idx = 0; idx < variantsCount; idx++) {
    if (idx + 1 == variantsCount) {
      clearPiping(indent);
      setLastChild();
    }

    printPiping();
    printDecl("EnumVariant", decl->variants[idx], 
        decl->getType()->toString(), true);
  }

  resetLastChild();
}

void ASTPrinter::visit(FieldDecl *decl) {
  printPiping();
  printDecl(decl->span, "FieldDecl", decl->name, decl->T->toString(), false);

  if (decl->isMutable()) {
    cout << " mut";
  }
  cout << '\n';
}

void ASTPrinter::visit(StructDecl *decl) {
  printPiping();
  printDecl(decl->span, "StructDecl", decl->name, "", false);

  if (decl->isPrivate()) {
    cout << " private\n";
  } else {
    cout << '\n';
  }

  increaseIndent();
  setPiping(indent);
  size_t fieldsCount = decl->fields.size();
  for (unsigned idx = 0; idx < fieldsCount; idx++) {
    if (idx + 1 == fieldsCount) {
      clearPiping(indent);
      setLastChild();
    }
    decl->fields[idx]->pass(this);
  }

  resetLastChild();
}

void ASTPrinter::visit(ImplicitCastExpr *expr) {
  printPiping();
  printExpr(expr->span, "ImplicitCastExpr", expr->T->toString());

  setLastChild();
  increaseIndent();
  expr->expr->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(ExplicitCastExpr *expr)  {
  printPiping();
  printExpr(expr->span, "ExplicitCastExpr", expr->T->toString());

  setLastChild();
  increaseIndent();
  expr->expr->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(DeclRefExpr *expr) {
  printPiping();
  printExpr(expr->span, "DeclRefExpr", expr->T->toString(), expr->ident, false);

  if (expr->getSpecifier() != "")
    cout << literalColor << " '" << expr->getSpecifier() << '\'' << clear;

  cout << '\n';
}

void ASTPrinter::visit(CallExpr *expr) {
  printPiping();
  printExpr(expr->span, "CallExpr", expr->T->toString(), expr->ident);

  resetLastChild();
  increaseIndent();
  for (unsigned idx = 0; idx < expr->getNumArgs(); idx++) {
    if (idx + 1 == expr->getNumArgs()) {
      clearPiping(indent);
      setLastChild();
    }

    expr->args[idx]->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(UnaryExpr *expr) {
  printPiping();
  printExpr(expr->span, "UnaryExpr", expr->T->toString(), "", false);
  cout << ' ' << literalColor << unaryOpToString(expr->op) << clear << '\n';

  unsigned topIndent = indent;
  setLastChild();
  increaseIndent();
  expr->base->pass(this);
  indent = topIndent;
  resetLastChild();
}

void ASTPrinter::visit(BinaryExpr *expr) {
  printPiping();
  printExpr(expr->span, "BinaryExpr", expr->T->toString(), "", false);
  if (expr->isAssignment()) {
    cout << " lvalue";
  } else if (expr->isComparison()) {
    cout << " comp";
  }

  cout << ' ' << literalColor << binaryOpToString(expr->op) << clear << '\n';

  unsigned topIndent = indent;
  resetLastChild();
  increaseIndent();
  expr->lhs->pass(this);
  setLastChild();
  expr->rhs->pass(this);
  indent = topIndent;
  resetLastChild();
}

void ASTPrinter::visit(BooleanLiteral *expr) {
  printPiping();
  printExpr(expr->span, "BooleanLiteral", expr->T->toString(), "", false);
  cout << ' ' << literalColor << expr->value << clear << '\n';
}

void ASTPrinter::visit(IntegerLiteral *expr) {
  printPiping();
  printExpr(expr->span, "IntegerLiteral", expr->T->toString(), "", false);
  cout << ' ' << literalColor << expr->value << clear << '\n';
}

void ASTPrinter::visit(FPLiteral *expr) {
  printPiping();
  printExpr(expr->span, "FPLiteral", expr->T->toString(), "", false);
  cout << ' ' << literalColor << expr->value << clear << '\n';
}

void ASTPrinter::visit(CharLiteral *expr) {
  printPiping();
  printExpr(expr->span, "CharLiteral", expr->T->toString(), "", false);
  cout << ' ' << literalColor << expr->value << clear << '\n';
}

void ASTPrinter::visit(StringLiteral *expr) {
  printPiping();
  printExpr(expr->span, "StringLiteral", expr->T->toString(), "", false);
  cout << ' ' << literalColor << expr->value << clear << '\n';
}

void ASTPrinter::visit(NullExpr *expr) {
  printPiping();
  printExpr(expr->span, "NullExpr", expr->T->toString());
}

void ASTPrinter::visit(ArrayExpr *expr) {
  printPiping();
  printExpr(expr->span, "ArrayExpr", expr->T->toString());
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  for (unsigned idx = 0; idx < expr->exprs.size(); idx++) {
    if (idx + 1 == expr->exprs.size()) {
      clearPiping(indent);
      setLastChild();
    }

    expr->exprs[idx]->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(ArraySubscriptExpr *expr) {
  printPiping();
  printExpr(expr->span, "ArraySubscriptExpr", expr->T->toString(), expr->name);
  resetLastChild();
  increaseIndent();
  expr->base->pass(this);
  setLastChild();
  expr->index->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(StructInitExpr *expr) {
  printPiping();
  printExpr(expr->span, "StructInitExpr", expr->T->toString(), expr->name);
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  for (unsigned idx = 0; idx < expr->fields.size(); idx++) {
    if (idx + 1 == expr->fields.size()) {
      clearPiping(indent);
      setLastChild();
    }

    expr->fields[idx].second->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(MemberExpr *expr) { 
  printPiping();
  printExpr(expr->span, "MemberExpr", expr->T->toString(), expr->member);
  setLastChild();
  increaseIndent();
  expr->base->pass(this);
  resetLastChild();
  decreaseIndent();
}

void ASTPrinter::visit(BreakStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "BreakStmt");
}

void ASTPrinter::visit(ContinueStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "ContinueStmt");
}

void ASTPrinter::visit(CompoundStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "CompoundStmt");
  increaseIndent();
  resetLastChild();

  setPiping(indent);
  for (unsigned idx = 0; idx < stmt->stmts.size(); idx++) {
    if (idx + 1 == stmt->stmts.size()) {
      clearPiping(indent);
      setLastChild();
    }
  
    stmt->stmts[idx]->pass(this);
  }

  decreaseIndent();
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

void ASTPrinter::visit(IfStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "IfStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->cond->pass(this);

  if (stmt->elseStmt) {
    stmt->thenStmt->pass(this);
    setLastChild();
    clearPiping(indent);
    stmt->elseStmt->pass(this);
  } else {
    setLastChild();
    clearPiping(indent);
    stmt->thenStmt->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(WhileStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "WhileStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->cond->pass(this);
  setLastChild();
  clearPiping(indent);
  stmt->body->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(UntilStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "UntilStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->cond->pass(this);
  setLastChild();
  clearPiping(indent);
  stmt->body->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(CaseStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "CaseStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->expr->pass(this);
  setLastChild();
  clearPiping(indent);
  stmt->body->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(DefaultStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "DefaultStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->body->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(MatchStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "MatchStmt");
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->expr->pass(this);

  for (unsigned idx = 0; idx < stmt->cases.size(); idx++) {
    if (idx + 1 == stmt->cases.size()) {
      clearPiping(indent);
      setLastChild();
    }

    stmt->cases[idx]->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(RetStmt *stmt) {
  printPiping();
  printStmt(stmt->span, "RetStmt");
  setLastChild();
  increaseIndent();
  stmt->expr->pass(this);
  resetLastChild();
}
