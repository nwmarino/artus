//>==- ASTPrinter.cpp ------------------------------------------------------<//
//
// The following source implements an AST pretty printer.
//
//>==----------------------------------------------------------------------==<//

#include <cstring>
#include <iostream>

#include "../../include/AST/ASTPrinter.h"
#include "../../include/AST/Decl.h"
#include "../../include/AST/Expr.h"

using namespace artus;

/// Returns a string representation of a unary operator.
inline static std::string unaryOpToString(UnaryExpr::UnaryOp op) {
  switch (op) {
  case UnaryExpr::UnaryOp::Negative: 
    return "-";
  case UnaryExpr::UnaryOp::Not: 
    return "!";
  case UnaryExpr::UnaryOp::Ref: 
    return "&";
  case UnaryExpr::UnaryOp::DeRef: 
    return "#";
  default: 
    return " ";
  }
}

/// Returns a string representation of a binary operator.
inline static std::string binaryOpToString(BinaryExpr::BinaryOp op) {
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
  if (pipingState.find(indent) == pipingState.end())
    return;

  pipingState[indent] = true;
}

inline void ASTPrinter::clearPiping(unsigned indent) 
{ pipingState[indent] = false; }

inline void ASTPrinter::printPiping() {
  std::string str = pipeColor;
  for (unsigned idx = 0; idx < indent; idx++)
    str = pipingState.find(idx) != pipingState.end() && pipingState[idx]
          ? str + "│ " : str + "  ";

  std::cout << str;

  if (isLastChild)
    std::cout << "`-" << clear;
  else
    std::cout << "|-" << clear;
}

inline void ASTPrinter::increaseIndent() { indent++; }

inline void ASTPrinter::decreaseIndent() { indent--;}

inline void ASTPrinter::flipLastChild() { isLastChild = !isLastChild; }

inline void ASTPrinter::setLastChild() { isLastChild = 1; }

inline void ASTPrinter::resetLastChild() { isLastChild = 0; }

const std::string ASTPrinter::spanToString(const Span &span) {
  return '<' + spanColor + span.begin.file + clear + '<' + spanColor
         + std::to_string(span.begin.line) + ':'
         + std::to_string(span.begin.col) + clear + ", " + spanColor
         + std::to_string(span.end.line) + ':'
         + std::to_string(span.end.col) + clear + ">>";
}

void ASTPrinter::printDecl(const std::string &node, const std::string &name,
                           const std::string &type, bool newl) {
  std::cout << declColor << node << clear
            << std::string(name.empty() ? 0 : 1, ' ') << nameColor << name 
            << clear;

  if (!type.empty())
    std::cout << typeColor << " '" << type << "'" << clear;

  if (newl)
    std::cout << '\n';
}

void ASTPrinter::printDecl(const Span &span, const std::string &node, 
                           const std::string &name, const std::string &type,
                           bool newl) {
  std::cout << declColor << node << clear << ' ' << spanToString(span)
            << std::string(name.empty() ? 0 : 1, ' ') << nameColor << name 
            << clear;

  if (!type.empty())
    std::cout << typeColor << " '" << type << '\'' << clear;

  if (newl)
    std::cout << '\n';                   
}

void ASTPrinter::printExpr(const Span &span, const std::string &node, 
                           const std::string &type, const std::string &ident, 
                           bool newl) {
  std::cout << exprColor << node << clear << ' ' << spanToString(span)
            << std::string(ident.empty() ? 0 : 1, ' ') << nameColor 
            << ident << clear;

  if (!type.empty())
    std::cout << typeColor << " '" << type << '\'' << clear;

  if (newl)
    std::cout << '\n';
}

void ASTPrinter::printStmt(const Span &span, const std::string &node, 
                           const std::string &name, bool newl) {
  std::cout << stmtColor << node << clear << ' ' << spanToString(span)
            << std::string(name.empty() ? 0 : 1, ' ') << nameColor << name
            << clear;

  if (newl) 
    std::cout << '\n';
}

void ASTPrinter::visit(PackageUnitDecl *decl) {
  printDecl("PackageUnitDecl", decl->getIdentifier(), "");
  
  setPiping(indent);
  std::size_t declCount = decl->decls.size();
  for (unsigned idx = 0; idx < declCount; idx++) {
    if (idx + 1 == declCount) {
      clearPiping(indent);
      setLastChild();
    }

    decl->decls.at(idx)->pass(this);
    indent = 0;
  }

  resetLastChild();
}

void ASTPrinter::visit(ImportDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "ImportDecl", decl->getPath().curr);
}

void ASTPrinter::visit(FunctionDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "FunctionDecl", decl->getName(),
      decl->getType()->toString(), false);

  if (decl->isPrivate())
    std::cout << " private\n";
  else
    std::cout << '\n';

  increaseIndent();
  std::size_t paramsCount = decl->getNumParams();
  for (unsigned idx = 0; idx < paramsCount; idx++)
    decl->params.at(idx)->pass(this);

  setLastChild();
  decl->body->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(ParamVarDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "ParamVarDecl", decl->getName(),
      decl->T->toString());
}

void ASTPrinter::visit(VarDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "VarDecl", decl->getName(), 
      decl->getType()->toString(), false);

  if (decl->isMutable())
    std::cout << " mut";

  std::cout << '\n';
  setLastChild();
  increaseIndent();
  decl->init->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(EnumDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "EnumDecl", decl->getName(), "", false);

  if (decl->isPrivate())
    std::cout << " private";

  std::cout << '\n';
  increaseIndent();
  setPiping(indent);
  std::size_t variantsCount = decl->getNumVariants();
  for (unsigned idx = 0; idx < variantsCount; idx++) {
    if (idx + 1 == variantsCount) {
      clearPiping(indent);
      setLastChild();
    }

    printPiping();
    printDecl("EnumVariant", decl->variants.at(idx), 
        decl->getType()->toString(), true);
  }

  resetLastChild();
}

void ASTPrinter::visit(FieldDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "FieldDecl", decl->getName(), 
      decl->getType()->toString(), false);

  if (decl->isMutable())
    std::cout << " mut";

  std::cout << '\n';
}

void ASTPrinter::visit(StructDecl *decl) {
  printPiping();
  printDecl(decl->getSpan(), "StructDecl", decl->getName(), "", false);

  if (decl->isPrivate())
    std::cout << " private";

  std::cout << '\n';
  increaseIndent();
  setPiping(indent);
  std::size_t fieldsCount = decl->getNumFields();
  for (unsigned idx = 0; idx < fieldsCount; idx++) {
    if (idx + 1 == fieldsCount) {
      clearPiping(indent);
      setLastChild();
    }
  
    decl->fields.at(idx)->pass(this);
  }

  resetLastChild();
}

void ASTPrinter::visit(ImplicitCastExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "ImplicitCastExpr", expr->getType()->toString());

  setLastChild();
  increaseIndent();
  expr->getExpr()->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(ExplicitCastExpr *expr)  {
  printPiping();
  printExpr(expr->getSpan(), "ExplicitCastExpr", expr->getType()->toString());

  setLastChild();
  increaseIndent();
  expr->getExpr()->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(DeclRefExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "DeclRefExpr", expr->getType()->toString(),
      expr->getIdent(), false);

  if (expr->hasSpecifier())
    std::cout << literalColor << " '" << expr->getSpecifier() << '\'' << clear;

  std::cout << '\n';
}

void ASTPrinter::visit(CallExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "CallExpr", expr->getType()->toString(), 
      expr->getIdent());

  resetLastChild();
  increaseIndent();
  for (unsigned idx = 0; idx < expr->getNumArgs(); idx++) {
    if (idx + 1 == expr->getNumArgs()) {
      clearPiping(indent);
      setLastChild();
    }

    expr->args.at(idx)->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(UnaryExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "UnaryExpr", expr->getType()->toString(), "", 
      false);
  std::cout << ' ' << literalColor << unaryOpToString(expr->getOp()) << clear 
            << '\n';

  unsigned topIndent = indent;
  setLastChild();
  increaseIndent();
  expr->getExpr()->pass(this);
  indent = topIndent;
  resetLastChild();
}

void ASTPrinter::visit(BinaryExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "BinaryExpr", expr->getType()->toString(), "",
      false);
  if (expr->isAssignment())
    std::cout << " lvalue";
  else if (expr->isComparison())
    std::cout << " comp";

  std::cout << ' ' << literalColor << binaryOpToString(expr->op) << clear << '\n';

  unsigned topIndent = indent;
  resetLastChild();
  increaseIndent();
  expr->getLHS()->pass(this);
  setLastChild();
  expr->getRHS()->pass(this);
  indent = topIndent;
  resetLastChild();
}

void ASTPrinter::visit(BooleanLiteral *expr) {
  printPiping();
  printExpr(expr->getSpan(), "BooleanLiteral", expr->getType()->toString(), 
      "", false);
  std::cout << ' ' << literalColor << expr->getValue() << clear << '\n';
}

void ASTPrinter::visit(IntegerLiteral *expr) {
  printPiping();
  printExpr(expr->getSpan(), "IntegerLiteral", expr->getType()->toString(), 
      "", false);
  std::cout << ' ' << literalColor << expr->getValue() << clear << '\n';
}

void ASTPrinter::visit(FPLiteral *expr) {
  printPiping();
  printExpr(expr->getSpan(), "FPLiteral", expr->getType()->toString(), 
      "", false);
  std::cout << ' ' << literalColor << expr->getValue() << clear << '\n';
}

void ASTPrinter::visit(CharLiteral *expr) {
  printPiping();
  printExpr(expr->getSpan(), "CharLiteral", expr->getType()->toString(), 
      "", false);
  std::cout << ' ' << literalColor << expr->getValue() << clear << '\n';
}

void ASTPrinter::visit(StringLiteral *expr) {
  printPiping();
  printExpr(expr->getSpan(), "StringLiteral", expr->getType()->toString(), 
      "", false);
  std::cout << ' ' << literalColor << expr->getValue() << clear << '\n';
}

void ASTPrinter::visit(NullExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "NullExpr", expr->getType()->toString());
}

void ASTPrinter::visit(ArrayExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "ArrayExpr", expr->getType()->toString());
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  for (unsigned idx = 0; idx < expr->getNumExprs(); idx++) {
    if (idx + 1 == expr->getNumExprs()) {
      clearPiping(indent);
      setLastChild();
    }

    expr->exprs.at(idx)->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(ArraySubscriptExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "ArraySubscriptExpr", expr->getType()->toString(),
      expr->getIdentifier());
  resetLastChild();
  increaseIndent();
  expr->base->pass(this);
  setLastChild();
  expr->index->pass(this);
  resetLastChild();
}

void ASTPrinter::visit(StructInitExpr *expr) {
  printPiping();
  printExpr(expr->getSpan(), "StructInitExpr", expr->getType()->toString(), 
      expr->name);
  resetLastChild();
  increaseIndent();
  setPiping(indent);
  for (unsigned idx = 0; idx < expr->getNumFields(); idx++) {
    if (idx + 1 == expr->getNumFields()) {
      clearPiping(indent);
      setLastChild();
    }

    expr->fields.at(idx).second->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(MemberExpr *expr) { 
  printPiping();
  printExpr(expr->getSpan(), "MemberExpr", expr->getType()->toString(), 
      expr->getMember());
  setLastChild();
  increaseIndent();
  expr->base->pass(this);
  resetLastChild();
  decreaseIndent();
}

void ASTPrinter::visit(BreakStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "BreakStmt");
}

void ASTPrinter::visit(ContinueStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "ContinueStmt");
}

void ASTPrinter::visit(CompoundStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "CompoundStmt");
  increaseIndent();
  resetLastChild();

  setPiping(indent);
  for (unsigned idx = 0; idx < stmt->stmts.size(); idx++) {
    if (idx + 1 == stmt->stmts.size()) {
      clearPiping(indent);
      setLastChild();
    }
  
    stmt->stmts.at(idx)->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(DeclStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "DeclStmt");
  setLastChild();
  increaseIndent();
  stmt->decl->pass(this);
  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(IfStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "IfStmt");

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
  printStmt(stmt->getSpan(), "WhileStmt");

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
  printStmt(stmt->getSpan(), "UntilStmt");

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
  printStmt(stmt->getSpan(), "CaseStmt");

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
  printStmt(stmt->getSpan(), "DefaultStmt");

  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->body->pass(this);

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(MatchStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "MatchStmt");

  resetLastChild();
  increaseIndent();
  setPiping(indent);
  stmt->expr->pass(this);

  for (unsigned idx = 0; idx < stmt->cases.size(); idx++) {
    if (idx + 1 == stmt->cases.size()) {
      clearPiping(indent);
      setLastChild();
    }

    stmt->cases.at(idx)->pass(this);
  }

  decreaseIndent();
  resetLastChild();
}

void ASTPrinter::visit(RetStmt *stmt) {
  printPiping();
  printStmt(stmt->getSpan(), "RetStmt");

  setLastChild();
  increaseIndent();
  stmt->expr->pass(this);

  resetLastChild();
}
