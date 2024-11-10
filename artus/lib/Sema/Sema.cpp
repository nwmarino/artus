#include "../../include/Core/Logger.h"
#include "../../include/Sema/Sema.h"

using namespace artus;

/// Semantic Analysis over a PackageUnitDecl.
void Sema::visit(PackageUnitDecl *decl) {
  
}

/// Semantic Analysis over a FunctionDecl.
void Sema::visit(FunctionDecl *decl) {

}

/// Semantic Analysis over a LabelDecl.
void Sema::visit(LabelDecl *decl) {

}

/// Semantic Analysis over an IntegerLiteral.
void Sema::visit(IntegerLiteral *expr) {

}

/// Semantic Analysis over a CompoundStmt.
void Sema::visit(CompoundStmt *stmt) {

}

/// Semantic Analysis over a LabelStmt.
void Sema::visit(LabelStmt *stmt) {

}

/// Semantic Analysis over a RetStmt.
///
/// This pass verifies that the return statement is well-formed and that the
/// return type matches the function's return type.
void Sema::visit(RetStmt *stmt) {

}
