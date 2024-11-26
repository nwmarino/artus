//>==- Stmt.cpp -----------------------------------------------------------<==//
//
// The following source contains implementations for classes defined in
// Stmt.h
//
//>==----------------------------------------------------------------------==<//

#include "../../include/AST/Decl.h"
#include "../../include/AST/Expr.h"
#include "../../include/AST/Stmt.h"
#include "../../include/Sema/Scope.h"

using namespace artus;

//>==- ValueStmt Implementation -------------------------------------------==<//
//
// The type of a value statement is non-owning, instead it is managed by the
// context type table.
//
//>==----------------------------------------------------------------------==<//

ValueStmt::ValueStmt(const Type *T, const Span &span) : Stmt(span),T(T) {}

//>==- CompoundStmt Implemntation -----------------------------------------==<//
//
// Compound statements may appear as follows:
//
// compound-stmt:
//      | '{' [stmt-list] '}'
//
// stmt-list:
//      | stmt
//      | stmt-list stmt
//
//>==----------------------------------------------------------------------==<//

CompoundStmt::CompoundStmt(std::vector<std::unique_ptr<Stmt>> stmts, 
                           Scope *scope, const Span &span)
    : Stmt(span), stmts(std::move(stmts)), scope(scope) {}

CompoundStmt::~CompoundStmt() { delete this->scope; }

const std::vector<Stmt *> CompoundStmt::getStmts() const {
  std::vector<Stmt *> stmts = {};
  for (const std::unique_ptr<Stmt> &stmt : this->stmts)
    stmts.push_back(stmt.get());

  return stmts;
}

//>==- DeclStmt Implementation --------------------------------------------==<//
//
// Declaration statements may appear as follows:
//
// decl-stmt:
//      | var-decl
//
//>==----------------------------------------------------------------------==<//

DeclStmt::DeclStmt(std::unique_ptr<Decl> decl, const Span &span) 
    : Stmt(span), decl(std::move(decl)) {}

//>==- IfStmt Implementation ----------------------------------------------==<//
//
// If statements may appear as follows:
//
// if-stmt:
//      | 'if' <expr> <stmt>
//      | 'if' <expr> <stmt> 'else' <stmt>
//
//>==----------------------------------------------------------------------==<//

IfStmt::IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> thenStmt, 
               std::unique_ptr<Stmt> elseStmt, const Span &span)
    : Stmt(span), cond(std::move(cond)), thenStmt(std::move(thenStmt)),
      elseStmt(std::move(elseStmt)) {}

bool IfStmt::hasElse() const { return elseStmt != nullptr; }

//>==- WhileStmt Implementation -------------------------------------------==<//
//
// While statements may appear as follows:
//
// while-stmt:
//      | 'while' <expr> <stmt>
//
//>==----------------------------------------------------------------------==<//

WhileStmt::WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body, 
                     const Span &span)
    : Stmt(span), cond(std::move(cond)), body(std::move(body)) {}

//>==- UntilStmt Implementation -------------------------------------------==<//
//
// Until statements may appear as follows:
//
// until-stmt:
//      | 'until' <expr> <stmt>
//
//>==----------------------------------------------------------------------==<//

UntilStmt::UntilStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body, 
                     const Span &span)
    : Stmt(span), cond(std::move(cond)), body(std::move(body)) {}

//>==- MatchCase Implementation -------------------------------------------==<//
//
// Match cases may appear as follows:
//
// match-case:
//      | case-stmt
//      | default-stmt
//
//>==----------------------------------------------------------------------==<//

MatchCase::MatchCase(std::unique_ptr<Stmt> body, const Span &span) 
    : Stmt(span), body(std::move(body)) {}

//>==- CaseStmt Implementation --------------------------------------------==<//
//
// Case statements may appear as follows:
//
// case-stmt:
//      | <expr> => <stmt>
//
//>==----------------------------------------------------------------------==<//

CaseStmt::CaseStmt(std::unique_ptr<Expr> expr, std::unique_ptr<Stmt> body, 
                   const Span &span) 
    : MatchCase(std::move(body), span), expr(std::move(expr)) {}

bool CaseStmt::isDefault() const { return false; }

//>==- DefaultStmt Implementation -----------------------------------------==<//
//
// Default statements may appear as follows:
//
// default-stmt:
//      | '_' => <stmt>
//
//>==----------------------------------------------------------------------==<//

DefaultStmt::DefaultStmt(std::unique_ptr<Stmt> body, const Span &span) 
    : MatchCase(std::move(body), span) {}

bool DefaultStmt::isDefault() const { return true; }

//>==- MatchStmt Implementation -------------------------------------------==<//
//
// Match statements may appear as follows:
//
// match-stmt:
//      | 'match' <expr> '{' [match-case-list] '}'
//
// match-case-list:
//      | match-case [',' match-case-list]
//
//>==----------------------------------------------------------------------==<//

MatchStmt::MatchStmt(std::unique_ptr<Expr> expr,
                     std::vector<std::unique_ptr<MatchCase>> cases, const Span &span)
    : Stmt(span), expr(std::move(expr)), cases(std::move(cases)) {}

bool MatchStmt::hasDefault() const {
  for (const std::unique_ptr<MatchCase> &c : cases) {
    MatchCase *mc = dynamic_cast<MatchCase *>(c.get());
    if (mc && mc->isDefault())
      return true;
  }

  return false;
}

DefaultStmt *MatchStmt::getDefault() const {
  for (const std::unique_ptr<MatchCase> &c : cases) {
    MatchCase *mc = dynamic_cast<MatchCase *>(c.get());
    if (mc && mc->isDefault())
      return dynamic_cast<DefaultStmt *>(mc);
  }

  return nullptr;
}

//>==- RetStmt Implementation ---------------------------------------------==<//
//
// Return statements may appear as follows:
//
// ret-stmt:
//      | 'ret' <expr>
//
//>==----------------------------------------------------------------------==<//

RetStmt::RetStmt(std::unique_ptr<Expr> expr, const Span &span)
    : ValueStmt(expr->getType(), span), expr(std::move(expr)) {}
