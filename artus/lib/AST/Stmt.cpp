#include "../../include/AST/Expr.h"

using std::string;
using std::vector;

using namespace artus;

/* Stmt Implementation ----------------------------------------------------===*/

Stmt::Stmt(const Span &span) : span(span) {}

const Span &Stmt::getSpan() const { return span; }

/* ValueStmt Implementation -----------------------------------------------===*/

ValueStmt::ValueStmt(const Type *T, const Span &span) : Stmt(span), T(T) {}

const Type *ValueStmt::getType() const { return T; }

/* CompoundStmt Implementation --------------------------------------------===*/

CompoundStmt::CompoundStmt(vector<std::unique_ptr<Stmt>> stmts, Scope *scope, 
                           const Span &span)
    : Stmt(span), stmts(std::move(stmts)), scope(scope) {}

void CompoundStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

const vector<std::unique_ptr<Stmt>> &CompoundStmt::getStmts() const { 
  return stmts; 
}

Scope *CompoundStmt::getScope() const { return scope; }

/* DeclStmt Implementation ------------------------------------------------===*/

DeclStmt::DeclStmt(std::unique_ptr<Decl> decl, const Span &span) 
    : Stmt(span), decl(std::move(decl)) {}

void DeclStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

/* IfStmt Implementation --------------------------------------------------===*/

IfStmt::IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenStmt, 
               std::unique_ptr<Stmt> elseStmt, const Span &span)
    : Stmt(span), cond(std::move(cond)), thenStmt(std::move(thenStmt)),
      elseStmt(std::move(elseStmt)) {}

void IfStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

bool IfStmt::hasElse() const { return elseStmt != nullptr; }

/* LabelStmt Implementation -----------------------------------------------===*/

LabelStmt::LabelStmt(const string &name, const Decl *decl, const Span &span) 
    : Stmt(span), name(name), decl(decl) {}

void LabelStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

const string &LabelStmt::getName() const { return name; }

const Decl *LabelStmt::getDecl() const { return decl; }

void LabelStmt::setDecl(const Decl *decl) { this->decl = decl; }

/* JmpStmt Implementation -------------------------------------------------===*/

JmpStmt::JmpStmt(const string &name, const Decl *decl, const Span &span) 
    : Stmt(span), name(name), decl(decl) {}

void JmpStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

/* RetStmt Implementation -------------------------------------------------===*/

RetStmt::RetStmt(std::unique_ptr<Expr> expr, const Span &span)
    : ValueStmt(expr->getType(), span), expr(std::move(expr)) {}

void RetStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Expr *RetStmt::getExpr() const { return expr.get(); }
