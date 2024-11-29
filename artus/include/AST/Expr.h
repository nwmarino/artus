//>==- Expr.h -------------------------------------------------------------==<//
//
// This header file declares inline expression AST nodes parsed from source.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_EXPR_H
#define ARTUS_AST_EXPR_H

#include <memory>
#include <string>
#include <vector>

#include "ASTPrinter.h"
#include "Stmt.h"
#include "../Codegen/Codegen.h"
#include "../Core/SourceLocation.h"
#include "../Sema/Type.h"

namespace artus {

/// Base class for all Expression nodes. Expressions are also statements.
class Expr : public ValueStmt {
protected:
  /// If 1, then this expression is a valid lvalue.
  bool lvalue : 1;

public:
  Expr(const Type *T, const Span &span) : ValueStmt(T, span), lvalue(false) {}
  ~Expr() = default;

  /// Returns true if this expression is an lvalue.
  bool isLValue() const { return lvalue; }
};

/// Base class for all Cast Expressions.
class CastExpr : public Expr {
  friend class Codegen;

protected:
  /// The expression to cast.
  std::unique_ptr<Expr> expr;

  /// The identifier of the type cast.
  const std::string ident;

public:
  CastExpr(std::unique_ptr<Expr> expr, const std::string &ident, const Type *T, 
           const Span &span);
  ~CastExpr() = default;

  /// \returns The target identifier of this cast.
  const std::string &getIdent() const { return ident; }

  /// \returns The base expression to cast.
  Expr *getExpr() const { return expr.get(); }
};

/// Represents an implicit type cast. 
///
/// This cast type is injected by the compiler after or during parse time to
/// from type mismatches or undefined behaviour.
class ImplicitCastExpr final : public CastExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  ImplicitCastExpr(std::unique_ptr<Expr> expr, const std::string &ident, 
                   const Type *T, const Span &span);
  ~ImplicitCastExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An explicit cast. For example, `i64 0`.
///
/// This cast type is explicitly stated by a source program.
class ExplicitCastExpr final : public CastExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  ExplicitCastExpr(std::unique_ptr<Expr> expr, const std::string &ident, 
                   const Type *T, const Span &span);
  ~ExplicitCastExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// A reference to a declaration. For example, `x`.
///
/// References may also include a nested specifier, for declarations such as
/// enums with multiple variants.
class DeclRefExpr : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The identifier of the referenced declaration.
  const std::string ident;

  /// The non-owning declaration being referenced.
  const Decl *decl;

  /// An optional name specifier for the reference.
  const std::string specifier;

public:
  DeclRefExpr(const std::string &ident, const Decl *decl, const Type *T, 
              const Span &span, const std::string &specifier = "");
  ~DeclRefExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The identifier of this reference.
  const std::string &getIdent() const { return ident; }

  /// \returns The name specifier of this reference.
  const std::string &getSpecifier() const { return specifier; }
  
  /// \returns `true` if this reference has a specifier.
  bool hasSpecifier() const { return specifier != ""; }
};

/// A call expression. For example, `@foo()`.
class CallExpr final : public DeclRefExpr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The arguments of the call.
  std::vector<std::unique_ptr<Expr>> args;

public:
  CallExpr(const std::string &callee, const Decl *decl, const Type *T,
           std::vector<std::unique_ptr<Expr>> args, const Span &span);
  ~CallExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The number of arguments of this call.
  std::size_t getNumArgs() const { return args.size(); }

  /// \returns The argument at the given index.
  Expr *getArg(size_t i) const { return args[i].get(); }

  /// \returns The arguments of this call.
  std::vector<Expr *> getArgs() const;
};

/// A unary expression. For example `-1`, `!true`, etc.
class UnaryExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  /// Possible kinds of unary operations.
  enum class UnaryOp {
    Unknown = -1,
    /// -
    Negative,
    /// !
    Not,
    /// &
    Ref,
    /// #
    DeRef,
  };

private:
  /// The base of the unary expression.
  std::unique_ptr<Expr> base;

  /// The operator of this unary expression.
  const UnaryOp op;

public:
  UnaryExpr(std::unique_ptr<Expr> base, UnaryOp op, const Span &span);
  ~UnaryExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The operator of this unary expression.
  UnaryOp getOp() const { return op; }

  /// \returns The base expression of this unary operator.
  Expr *getExpr() const { return base.get(); }
};

/// A binary expression. For example, `1 + 1`, `2 * 3`, etc.
class BinaryExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  /// Possible kinds of binary operations.
  enum class BinaryOp {
    Unknown = -1,
    // =
    Assign,
    // +=
    AddAssign,
    // -=
    SubAssign,
    // *=
    MultAssign,
    // /=
    DivAssign,
    // ==
    Equals,
    // !=
    NotEquals,
    // <
    LessThan,
    // >
    GreaterThan,
    // <=
    LessEquals,
    // >=
    GreaterEquals,
    // &&
    LogicalAnd,
    // ||
    LogicalOr,
    // ^^
    LogicalXor,
    // +
    Add,
    // -
    Sub,
    // *
    Mult,
    // /
    Div,
  };

private:
  /// The left-hand side of the binary expression.
  std::unique_ptr<Expr> lhs;

  /// The right-hand side of the binary expression.
  std::unique_ptr<Expr> rhs;

  /// The operator of this binary expression.
  const BinaryOp op;

public:
  BinaryExpr(std::unique_ptr<Expr> lhs, std::unique_ptr<Expr> rhs, 
             BinaryOp op, const Span &span);
  ~BinaryExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns `true` if this binary expression is a direct assignment: `=`
  bool isDirectAssignment() const { return op == BinaryOp::Assign; }

  /// \returns `true` if this binary expression is an assignment.
  bool isAssignment() const 
  { return op >= BinaryOp::Assign && op <= BinaryOp::DivAssign; }

  /// \returns `true` if this binary expression is a comparison.
  bool isComparison() const 
  { return op >= BinaryOp::Equals && op <= BinaryOp::LogicalXor; }

  /// \returns `true` if this binary expression is a logical comparison.
  bool isLogicComparison() const
  { return op >= BinaryOp::LogicalAnd && op <= BinaryOp::LogicalXor; }

  /// \returns `true` if this binary expression is a numerical comparison.
  bool isNumericalComparison() const
  { return op >= BinaryOp::Equals && op <= BinaryOp::GreaterEquals; }

  /// \returns The left hand side expression of this binary operator.
  Expr *getLHS() const { return lhs.get(); }

  /// \returns The right hand side expression of this binary operator.
  Expr *getRHS() const { return rhs.get(); }
};

/// A boolean literal; `true` or `false`.
class BooleanLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const bool value;

public:
  BooleanLiteral(bool value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}
  ~BooleanLiteral() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The value of this boolean literal.
  bool getValue() const { return value; }
};

/// An integer literal. For example, `0`, `1`, etc.
class IntegerLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const int value;

public:
  IntegerLiteral(int value, const Type *T, const Span &span) 
      : Expr(T, span), value(value) {}
  ~IntegerLiteral() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The value of this integer literal.
  int getValue() const { return value; }
};

/// A floating point literal. For example, `0.0`, `1.0`, etc.
class FPLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const double value;

public:
  FPLiteral(double value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}
  ~FPLiteral() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The value of this floating point literal.
  double getValue() const { return value; }
};

/// A character literal. For example, `'a'`, `'b'`, etc.
class CharLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const char value;

public:
  CharLiteral(char value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}
  ~CharLiteral() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The value of this character literal.
  char getValue() const { return value; }
};

/// A string literal. For example, `"hello"`, `"world"`, etc.
class StringLiteral final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The literal value nested in this node.
  const std::string value;

public:
  StringLiteral(const std::string &value, const Type *T, const Span &span)
      : Expr(T, span), value(value) {}
  ~StringLiteral() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The value of this string literal.
  const std::string &getValue() const { return value; }
};

/// A `null` expression.
class NullExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

public:
  NullExpr(const Type *T, const Span &span) : Expr(T, span) {}
  ~NullExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }
};

/// An array expression. For example, `[1, 2, 3]`.
class ArrayExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The list of expressions in the array.
  std::vector<std::unique_ptr<Expr>> exprs;

public:
  ArrayExpr(std::vector<std::unique_ptr<Expr>> exprs, const Type *T, 
            const Span &span);
  ~ArrayExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The number of expressions in this array.
  std::size_t getNumExprs() const { return exprs.size(); }

  /// \returns The expression at the given index.
  Expr *getExpr(size_t i) const { return exprs[i].get(); }

  /// \returns The expression in this array.
  std::vector<Expr *> getExprs() const;
};

/// An array subscript expression. For example, `arr[0]`.
class ArraySubscriptExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The identifier of the array base.
  const std::string identifier;

  /// The base of the array access.
  std::unique_ptr<Expr> base;

  /// The index of the array access.
  std::unique_ptr<Expr> index;

public:
  ArraySubscriptExpr(const std::string &identifier, std::unique_ptr<Expr> base,
                     std::unique_ptr<Expr> index, const Type *T, const Span &span);
  ~ArraySubscriptExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The base identifier of this expression.
  const std::string &getIdentifier() const { return identifier; }
};

/// A struct initialization expression. For example, `Foo { a: 1, b: 2 }`.
class StructInitExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The name of the struct.
  const std::string name;

  /// The list of field initializers.
  std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields;

  /// A non-owning reference to the struct declaration.
  const StructDecl *decl;

public:
  StructInitExpr(const std::string &name, const Type *T,
                 std::vector<std::pair<std::string, std::unique_ptr<Expr>>> fields, 
                 const Span &span);
  ~StructInitExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The name of the struct to be instantiated.
  const std::string &getName() const { return name; }

  /// \returns The number of fields in this struct.
  std::size_t getNumFields() const { return fields.size(); }

  /// \returns The field expression at the index \p i.
  Expr *getField(std::size_t i);

  /// \returns The name of the fields of this initialization expression.
  std::vector<std::string> getFieldNames() const;

  /// \returns The fields of this initialization expression.
  std::vector<Expr *> getFieldExprs() const;

  /// Sets the target struct declaration to \p d.
  void setDecl(const StructDecl *d) { decl = d; }
};

/// A struct member access expression. For example, `foo.bar`.
class MemberExpr final : public Expr {
  friend class ASTPrinter;
  friend class Codegen;
  friend class ReferenceAnalysis;
  friend class Sema;

  /// The base of the member access.
  std::unique_ptr<Expr> base;

  /// The name of the member.
  const std::string member;

  /// The index of the member in the base struct.
  int index = -1;

public:
  MemberExpr(std::unique_ptr<Expr> base, const std::string &member, 
             const Type *T, const Span &span);
  ~MemberExpr() = default;

  void pass(ASTVisitor *visitor) override { visitor->visit(this); }

  /// \returns The base expression of this member access.
  Expr *getBase() const { return base.get(); }

  /// \returns The identifier of the member of this access expression.
  const std::string &getMember() const { return member; }

  /// \returns The index of the target member in the base struct.
  int getIndex() const { return index; }

  /// Sets the index of this target member as \p idx in the base struct.
  void setIndex(int idx = -1) { index = idx; }
};

} // end namespace artus

#endif // ARTUS_AST_EXPR_H
