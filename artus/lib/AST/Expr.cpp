//>==- Expr.cpp -----------------------------------------------------------<==//
//
// The following source contains implementations for classes defined in Expr.h
//
//>==----------------------------------------------------------------------==<//

#include "../../include/AST/Decl.h"
#include "../../include/AST/Expr.h"
#include "../../include/Sema/Type.h"

#include <utility>

using namespace artus;

//>==- CastExpr Implementation --------------------------------------------==<//
//
// Cast expressions are used to represent type casts in the AST. Casts "own"
// the expression they target.
//
//>==----------------------------------------------------------------------==<//

CastExpr::CastExpr(std::unique_ptr<Expr> expr, const std::string &ident, 
                   const Type *T, const Span &span)
    : Expr(T, span), expr(std::move(expr)), ident(ident) {}

ImplicitCastExpr::ImplicitCastExpr(std::unique_ptr<Expr> expr, const std::string &ident, 
                                   const Type *T, const Span &span)
    : CastExpr(std::move(expr), ident, T, span) {}

//>==- ExplicitCastExpr Implementation ------------------------------------==<//
//
// Explicit casts are explicitly defined by a source program. They may appears
// as follows:
//
// explicit-cast:
//      | <identifier> <expr>
//
//>==----------------------------------------------------------------------==<//

ExplicitCastExpr::ExplicitCastExpr(std::unique_ptr<Expr> expr, 
                                   const std::string &ident,
                                   const Type *T, const Span &span)
    : CastExpr(std::move(expr), ident, T, span) {}

//>==- DeclRefExpr Implementation -----------------------------------------==<//
//
// Declaration reference expressions may appear as follows:
//
// decl-ref-expr:
//      | <identifier>
//      | <identifier> '::' <identifier>
//
//>==----------------------------------------------------------------------==<//

DeclRefExpr::DeclRefExpr(const std::string &ident, const Decl *decl, const Type *T, 
                         const Span &span, const std::string &specifier)
    : Expr(T, span), ident(ident), decl(decl), specifier(specifier) {}

//>==- CallExpr Implementation --------------------------------------------==<//
//
// Call expressions are special declaration refernce expressions that may
// contain arguments to a function or struct method. They may appear as follows:
//
// call-expr:
//      | '@' <identifier> '(' [expr-list] ')'
//
// expr-list:
//     | <expr> [',' <expr-list>]
//
//>==----------------------------------------------------------------------==<//

CallExpr::CallExpr(const std::string &callee, const Decl *decl, const Type *T,
                   std::vector<std::unique_ptr<Expr>> args, const Span &span)
    : DeclRefExpr(callee, decl, T, span), args(std::move(args)) {}

std::vector<Expr *> CallExpr::getArgs() const {
  std::vector<Expr *> exprs = {};
  for (const std::unique_ptr<Expr> &expr : args)
    exprs.push_back(expr.get());

  return exprs;
}

//>==- UnaryExpr Implementation -------------------------------------------==<//
//
// Unary expression may appear as follows:
//
// unary-expr:
//      | '-' <expr>
//      | '!' <expr>
//      | '&' <expr>
//      | '#' <expr>
//
//>==----------------------------------------------------------------------==<//

UnaryExpr::UnaryExpr(std::unique_ptr<Expr> base, UnaryOp op, const Span &span)
    : Expr(base->getType(), span), base(std::move(base)), op(op) {
  if (op == UnaryOp::DeRef)
    this->setLValue();
}

//>==- BinaryExpr Implementation ------------------------------------------==<//
//
// Binary expressions may appear as follows:
//
// binary-expr:
//      | <expr> '=' <expr>
//      | <expr> '+=' <expr>
//      | <expr> '-=' <expr>
//      | <expr> '*=' <expr>
//      | <expr> '/=' <expr>
//      | <expr> '==' <expr>
//      | <expr> '!=' <expr>
//      | <expr> '<' <expr>
//      | <expr> '>' <expr>
//      | <expr> '<=' <expr>
//      | <expr> '>=' <expr>
//      | <expr> '&&' <expr>
//      | <expr> '||' <expr>
//      | <expr> '^^' <expr>
//      | <expr> '+' <expr>
//      | <expr> '-' <expr>
//      | <expr> '*' <expr>
//      | <expr> '/' <expr>
//
//>==----------------------------------------------------------------------==<//

BinaryExpr::BinaryExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs, 
                       BinaryOp op, const Span &span)
    : Expr(lhs->getType(), span), lhs(std::move(lhs)), rhs(std::move(rhs)), 
      op(op) {}

//>==- ArrayExpr Implementation -------------------------------------------==<//
//
// Array expressions may appear as follows:
//
// array-expr:
//      | '[' [expr-list] ']'
//
// expr-list:
//      | <expr> [',' <expr-list>]
//
//>==----------------------------------------------------------------------==<//

ArrayExpr::ArrayExpr(std::vector<std::unique_ptr<Expr>> exprs, const Type *T, 
                     const Span &span)
    : Expr(T, span), exprs(std::move(exprs)) {}

std::vector<Expr *> ArrayExpr::getExprs() const {
  std::vector<Expr *> exprs = {};
  for (const std::unique_ptr<Expr> &expr : this->exprs)
    exprs.push_back(expr.get());

  return exprs;
}

//>==- ArraySubscriptExpr Implementation ----------------------------------==<//
//
// Array subscript expressions may appear as follows:
//
// array-subscript-expr:
//      | <identifier> '[' <expr> ']'
//
//>==----------------------------------------------------------------------==<//

ArraySubscriptExpr::ArraySubscriptExpr(const std::string &identifier, 
                                       std::unique_ptr<Expr> base, 
                                       std::unique_ptr<Expr> index, 
                                       const Type *T, const Span &span)
    : Expr(T, span), identifier(identifier), base(std::move(base)), 
      index(std::move(index)) {}

//>==- StructInitExpr Implementation --------------------------------------==<//
//
// Struct initialization expressions may appear as follows:
//
// struct-init-expr:
//      | <identifier> '{' [field-list] '}'
//
// field-list:
//      | <identifier> ':' <expr> [',' <field-list>]
//
//>==----------------------------------------------------------------------==<//

StructInitExpr::StructInitExpr(const std::string &name, const Type *T,
                               std::vector<std::pair<std::string, 
                               std::unique_ptr<Expr>>> fields, const Span &span)
    : Expr(T, span), name(name), fields(std::move(fields)) {}

Expr *StructInitExpr::getField(std::size_t i) 
{ return i < fields.size() ? fields[i].second.get() : nullptr; }

std::vector<std::string> StructInitExpr::getFieldNames() const {
  std::vector<std::string> names = {};
  for (const std::pair<std::string, std::unique_ptr<Expr>> &field : fields)
    names.push_back(field.first);

  return names;
}

std::vector<Expr *> StructInitExpr::getFieldExprs() const {
  std::vector<Expr *> exprs = {};
  for (const std::pair<std::string, std::unique_ptr<Expr>> &field : fields)
    exprs.push_back(field.second.get());

  return exprs;
}

//>==- MemberExpr Implementation ------------------------------------------==<//
//
// Member expressions may appear as follows:
//
// member-expr:
//      | <expr> '.' <identifier>
//
//>==----------------------------------------------------------------------==<//

MemberExpr::MemberExpr(std::unique_ptr<Expr> base, const std::string &member, 
                       const Type *T, const Span &span)
    : Expr(T, span), base(std::move(base)), member(member) {}
