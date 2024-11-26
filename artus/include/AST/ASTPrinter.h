//>==- ASTPrinter.h -------------------------------------------------------==<//
//
// This header file defines a visitor pattern upon an AST to dump its contents
// in a readable format.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_AST_ASTPRINTER_H
#define ARTUS_AST_ASTPRINTER_H

#include <map>
#include <string>

#include "ASTVisitor.h"
#include "../Core/SourceLocation.h"

namespace artus {

/// This class implements an AST printer that traverses the AST and "pretty"
/// prints it to standard output.
class ASTPrinter final : public ASTVisitor {
  /// Colour constants.
  const std::string clear = "\033[0m";
  const std::string clr_bold = "\033[1m";
  const std::string clr_italic = "\033[3m";
  const std::string clr_green = "\033[32m";
  const std::string clr_yellow = "\033[33m";
  const std::string clr_blue = "\033[34m";
  const std::string clr_purple = "\033[35m";
  const std::string clr_cyan = "\033[36m";

  const std::string declColor = clr_bold + clr_yellow;
  const std::string exprColor = clr_purple;
  const std::string stmtColor = clr_bold + clr_purple;
  const std::string nameColor = clr_italic + clr_blue;
  const std::string pipeColor = clr_blue;
  const std::string typeColor = clr_green;
  const std::string spanColor = clr_yellow;
  const std::string literalColor = clr_bold + clr_cyan;

  /// Map used to store the current piping state of the AST. Each key refers to
  /// an indentiation level, and the value corresponds to the state.
  std::map<unsigned, bool> pipingState;
  
  /// The current indentation level.
  unsigned int indent = 0;

  /// Flag used to indicate if the AST node is the last child.
  bool isLastChild : 1 = 0;

  /// Set the state to place piping at a target indent moving forward.
  inline void setPiping(unsigned indent);

  /// Clear the state to place piping at a target indent moving forward.
  inline void clearPiping(unsigned indent);

  /// Use the current piping state to print the appropriate piping and indent.
  inline void printPiping();

  /// Increase the indent by one level.
  inline void increaseIndent();

  /// Decrease the indent by one level.
  inline void decreaseIndent();

  /// Flips the current state of the `isLastChild` flag.
  inline void flipLastChild();

  /// Sets the current state of the `isLastChild` flag.
  inline void setLastChild();

  /// Clears the current state of the `isLastChild` flag.
  inline void resetLastChild();

  /// Returns a color-coded string representation of the provided span.
  inline const std::string spanToString(const Span &span);

  /// Print a declaration node with the provided data.
  void printDecl(const std::string &node, const std::string &name = "",
                 const std::string &type = "", bool newl = true);

  /// Print a declaration node with the provided data.
  void printDecl(const Span &span, const std::string &node,
                 const std::string &name = "", const std::string &type = "",
                 bool newl = true);

  /// Print an expression node with the provided data.
  void printExpr(const Span &span, const std::string &node,
                 const std::string &type, const std::string &ident = "",
                 bool newl = true);

  /// Print a statement node with the provided data.
  void printStmt(const Span &span, const std::string &node,
                 const std::string &name = "", bool newl = true);

public:
  void visit(PackageUnitDecl *decl) override;
  void visit(ImportDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(VarDecl *decl) override;
  void visit(EnumDecl *decl) override;
  void visit(FieldDecl *decl) override;
  void visit(StructDecl *decl) override;

  void visit(ImplicitCastExpr *expr) override;
  void visit(ExplicitCastExpr *expr) override;
  void visit(DeclRefExpr *expr) override;
  void visit(CallExpr *expr) override;
  void visit(UnaryExpr *expr) override;
  void visit(BinaryExpr *expr) override;
  void visit(BooleanLiteral *expr) override;
  void visit(IntegerLiteral *expr) override;
  void visit(FPLiteral *expr) override;
  void visit(CharLiteral *expr) override;
  void visit(StringLiteral *expr) override;
  void visit(NullExpr *expr) override;
  void visit(ArrayExpr *expr) override;
  void visit(ArraySubscriptExpr *expr) override;
  void visit(StructInitExpr *expr) override;
  void visit(MemberExpr *expr) override;

  void visit(BreakStmt *stmt) override;
  void visit(ContinueStmt *stmt) override;
  void visit(CompoundStmt *stmt) override;
  void visit(DeclStmt *stmt) override;
  void visit(IfStmt *stmt) override;
  void visit(WhileStmt *stmt) override;
  void visit(UntilStmt *stmt) override;
  void visit(CaseStmt *stmt) override;
  void visit(DefaultStmt *stmt) override;
  void visit(MatchStmt *stmt) override;
  void visit(RetStmt *stmt) override;
};

} // end namespace artus

#endif // ARTUS_AST_ASTPRINTER_H
