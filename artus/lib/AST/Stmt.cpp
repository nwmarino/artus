#include "Stmt.h"
#include "../../include/AST/Expr.h"
#include "ASTVisitor.h"

using std::string;
using std::vector;

using namespace artus;

/* Stmt Implementation ----------------------------------------------------===*/

Stmt::Stmt(const Span &span) : span(span) {}

const Span &Stmt::getSpan() const { return span; }

/* ValueStmt Implementation -----------------------------------------------===*/

ValueStmt::ValueStmt(const Type *T, const Span &span) : Stmt(span), T(T) {}

const Type *ValueStmt::getType() const { return T; }

/* BreakStmt Implementation -----------------------------------------------===*/

BreakStmt::BreakStmt(const Span &span) : Stmt(span) {}

void BreakStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

/* ContinueStmt Implementation --------------------------------------------===*/

ContinueStmt::ContinueStmt(const Span &span) : Stmt(span) {}

void ContinueStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

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

/* WhileStmt Implementation -----------------------------------------------===*/

WhileStmt::WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body, 
                     const Span &span)
    : Stmt(span), cond(std::move(cond)), body(std::move(body)) {}

void WhileStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

/* UntilStmt Implementation -----------------------------------------------===*/

UntilStmt::UntilStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body, 
                     const Span &span)
    : Stmt(span), cond(std::move(cond)), body(std::move(body)) {}

void UntilStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

/* MatchCase Implementation -----------------------------------------------===*/

MatchCase::MatchCase(std::unique_ptr<Stmt> body, const Span &span) 
    : Stmt(span), body(std::move(body)) {}

/* CaseStmt Implementation ------------------------------------------------===*/

CaseStmt::CaseStmt(std::unique_ptr<Expr> expr, std::unique_ptr<Stmt> body, 
                   const Span &span) 
    : MatchCase(std::move(body), span), expr(std::move(expr)) {}

void CaseStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

bool CaseStmt::isDefault() const { return false; }

/* DefaultStmt Implementation ---------------------------------------------===*/

DefaultStmt::DefaultStmt(std::unique_ptr<Stmt> body, const Span &span) 
    : MatchCase(std::move(body), span) {}

void DefaultStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

bool DefaultStmt::isDefault() const { return true; }

/* MatchStmt Implementation -----------------------------------------------===*/

MatchStmt::MatchStmt(std::unique_ptr<Expr> expr,
                     vector<std::unique_ptr<MatchCase>> cases, const Span &span)
    : Stmt(span), expr(std::move(expr)), cases(std::move(cases)) {}

void MatchStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

bool MatchStmt::hasDefault() const {
  for (const std::unique_ptr<MatchCase> &c : cases) {
    MatchCase *mc = dynamic_cast<MatchCase *>(c.get());
    if (mc && mc->isDefault()) {
      return true;
    }
  }
  return false;
}

DefaultStmt *MatchStmt::getDefault() const {
  for (const std::unique_ptr<MatchCase> &c : cases) {
    MatchCase *mc = dynamic_cast<MatchCase *>(c.get());
    if (mc && mc->isDefault()) {
      return dynamic_cast<DefaultStmt *>(mc);
    }
  }
  return nullptr;
}

/* RetStmt Implementation -------------------------------------------------===*/

RetStmt::RetStmt(std::unique_ptr<Expr> expr, const Span &span)
    : ValueStmt(expr->getType(), span), expr(std::move(expr)) {}

void RetStmt::pass(ASTVisitor *visitor) { visitor->visit(this); }

const Expr *RetStmt::getExpr() const { return expr.get(); }
