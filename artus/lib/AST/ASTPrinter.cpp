#include <iostream>

#include "ASTPrinter.h"
#include "Expr.h"

using std::cout;
using std::size_t;
using std::string;

using namespace artus;

inline void ASTPrinter::setPiping(unsigned indent) {
  if (pipingState.find(indent) == pipingState.end())
    pipingState[indent] = true;
}

inline void ASTPrinter::clearPiping(unsigned indent) {
  pipingState[indent] = false;
}

inline void ASTPrinter::printPiping() {
  string str = pipeColor;
  for (unsigned idx = 0; idx < indent; idx++) {
    str = pipingState.find(idx) != pipingState.end() && pipingState[idx] ? \
        str + "│ " : str + "  ";
  }

  if (isLastChild)
    cout << str << "`-" << CLEAR;
  else
    cout << str << "|-" << CLEAR;
}

inline void ASTPrinter::printIndent() { cout << string(indent * 2, ' '); }

inline void ASTPrinter::increaseIndent() { indent++; }

inline void ASTPrinter::decreaseIndent() { indent--;}

inline void ASTPrinter::flipLastChild() { isLastChild = ~isLastChild; }

void ASTPrinter::visit(PackageUnitDecl *decl) {
  cout << unitColor << "PackageUnitDecl " << CLEAR << nameColor << \
      decl->identifier << CLEAR << '\n';

  size_t declCount = decl->decls.size();
  for (unsigned idx = 0; idx < declCount; idx++) {
    decl->decls[idx]->pass(this);
    if (idx == declCount)
      flipLastChild();
  }

  flipLastChild();
}

void ASTPrinter::visit(FunctionDecl *decl) {
  cout << declColor << "FunctionDecl " << CLEAR << nameColor << \
      decl->getName() << CLEAR << qualType << '\'' << decl->T->toString() << \
      CLEAR << '\n';

  

  
}

void ASTPrinter::visit(ParamVarDecl *decl) {
  cout << declColor << "ParamVarDecl " << CLEAR << nameColor << \
      decl->getName() << CLEAR << qualType << '\'' << decl->T->toString() << \
      CLEAR << '\n';
}