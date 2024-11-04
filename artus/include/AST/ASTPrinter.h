#ifndef ARTUS_AST_PRINTER_H
#define ARTUS_AST_PRINTER_H

#include <map>
#include <string>

#include "ASTVisitor.h"

using std::map;
using std::string;

namespace artus {

/// This class implements an AST printer that traverses the AST and "pretty"
/// prints it to standard output.
class ASTPrinter final : public ASTVisitor {
  friend class PackageUnitDecl;
  friend class FunctionDecl;
  friend class LabelDecl;
  friend class IntegerLiteral;
  friend class CompoundStmt;
  friend class LabelStmt;
  friend class RetStmt;

  /// Colour constants.
  string CLEAR = "\033[0m";
  string BOLD = "\033[1m";
  string ITALIC = "\033[3m";
  string GREEN = "\033[32m";
  string YELLOW = "\033[33m";
  string BLUE = "\033[34m";
  string PURPLE = "\033[35m";
  string CYAN = "\033[36m";

  string unitColor = BOLD + YELLOW;
  string declColor = BOLD + GREEN;
  string exprColor = BOLD + PURPLE;
  string stmtColor = BOLD + PURPLE;
  string nameColor = BLUE;
  string pipeColor = BLUE;
  string qualType = GREEN;
  string refType = ITALIC + GREEN;
  string literalColor = BOLD + CYAN;

  /// Map used to store the current piping state of the AST. Each key refers to
  /// an indentiation level, and the value corresponds to the state.
  map<unsigned, bool> pipingState;
  
  /// The current indentation level.
  unsigned indent = 0;

  /// Flag used to indicate if the AST node is the last child.
  unsigned isLastChild : 1;

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

  void visit(PackageUnitDecl *decl) override;
  void visit(FunctionDecl *decl) override;
  void visit(ParamVarDecl *decl) override;
  void visit(LabelDecl *decl) override;

  void visit(IntegerLiteral *expr) override;

  void visit(CompoundStmt *stmt) override;
  void visit(LabelStmt *stmt) override;
  void visit(RetStmt *stmt) override;

public:
  ASTPrinter();
};

} // namespace artus

#endif // ARTUS_AST_PRINTER_H
