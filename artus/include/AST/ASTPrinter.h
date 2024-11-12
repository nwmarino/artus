#ifndef ARTUS_AST_ASTPRINTER_H
#define ARTUS_AST_ASTPRINTER_H

#include <map>
#include <string>

#include "ASTVisitor.h"

using std::map;
using std::string;

namespace artus {

/// This class implements an AST printer that traverses the AST and "pretty"
/// prints it to standard output.
class ASTPrinter final : public ASTVisitor {
  /// Colour constants.
  const string clear = "\033[0m";
  const string clr_bold = "\033[1m";
  const string clr_italic = "\033[3m";
  const string clr_green = "\033[32m";
  const string clr_yellow = "\033[33m";
  const string clr_blue = "\033[34m";
  const string clr_purple = "\033[35m";
  const string clr_cyan = "\033[36m";

  const string unitColor = clr_bold + clr_yellow;
  const string declColor = clr_bold + clr_green;
  const string exprColor = clr_bold + clr_purple;
  const string stmtColor = clr_bold + clr_purple;
  const string nameColor = clr_blue;
  const string pipeColor = clr_blue;
  const string typeColor = clr_green;
  const string literalColor = clr_bold + clr_cyan;

  /// Map used to store the current piping state of the AST. Each key refers to
  /// an indentiation level, and the value corresponds to the state.
  map<unsigned, bool> pipingState;
  
  /// The current indentation level.
  unsigned indent = 0;

  /// Flag used to indicate if the AST node is the last child.
  unsigned isLastChild : 1 = 0;

  /// Set the state to place piping at a target indent moving forward.
  inline void setPiping(unsigned indent);

  /// Clear the state to place piping at a target indent moving forward.
  inline void clearPiping(unsigned indent);

  /// Use the current piping state to print the appropriate piping and indent.
  inline void printPiping();

  /// Print the current indentation level.
  inline void printIndent();

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

public:
  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(LabelDecl *decl) override;

  void visit(ImplicitCastExpr *expr) override;
  void visit(ExplicitCastExpr *expr) override;
  void visit(BinaryExpr *expr) override;
  void visit(IntegerLiteral *expr) override;

  void visit(CompoundStmt *stmt) override;
  void visit(LabelStmt *stmt) override;
  void visit(JmpStmt *stmt) override;
  void visit(RetStmt *stmt) override;
};

} // namespace artus

#endif // ARTUS_AST_ASTPRINTER_H
