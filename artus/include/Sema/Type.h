#ifndef ARTUS_SEMA_TYPE_H
#define ARTUS_SEMA_TYPE_H

#include <string>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

using std::string;
using std::vector;

namespace artus {

/// Base class for all Types and their derivatives. Often nested in an 
/// expression node or typed declaration.
class Type {
public:
  virtual ~Type() = default;

  /// Returns true if the type is definitively an integer type, or can be
  /// evaluted to otherwise, and false otherwise.
  virtual bool isIntegerType() const = 0;

  /// Returns true if the type is definitively a floating point type, or can be
  /// evaluted to otherwise, and false otherwise.
  virtual bool isFloatingPointType() const = 0;

  /// Returns the bit width of the type. For example, an `i64` would return 64.
  virtual unsigned getBitWidth() const = 0;

  /// Returns a string representation of the type. This identifier is identical
  /// to the identifier parsed.
  virtual string toString() const = 0;

  /// Compare two types for equality. Returns 0 if the types are not equal, 1
  /// if they are exactly equivelant, and 2 if they are suitable for implicit
  /// conversion. For example, enums and primitive integers are implicitly
  /// convertible.
  virtual int compare(const Type *other) const = 0;

  /// Returns the equivelant LLVM type for the given type.
  virtual llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const = 0;
};

/// Class to represent basic, built-in types, such as `i64` or `char`.
class BasicType final : public Type {
  friend class Context;

  /// Possible kinds of basic types.
  enum BasicTypeKind {
    INT1 = 0,
    INT8,
    INT32,
    INT64,
    UINT8,
    UINT32,
    UINT64,
    FP64
  };

  /// The kind of basic type.
  const BasicTypeKind kind;

  /// Constructor for a basic type. Only called by the CContext class.
  BasicType(BasicTypeKind kind) : kind(kind) {}

public:
  /// Returns true if the basic type kind is an integer or a character, and
  /// false otherwise.
  bool isIntegerType() const override { return kind <= UINT64; }

  /// Returns true if the basic type kind is a floating point, and false
  /// otherwise.
  bool isFloatingPointType() const override { return kind == FP64; }

  /// Returns the bit width of the basic type. For example, an `i64` would
  /// return 64.
  unsigned getBitWidth() const override {
    switch (kind) {
      case INT1: return 1;
      case INT8: return 8;
      case INT32: return 32;
      case INT64: return 64;
      case UINT8: return 8;
      case UINT32: return 32;
      case UINT64: return 64;
      case FP64: return 64;
    }
    return 0;
  }

  /// Returns a string representation of the basic type.
  string toString() const override {
    switch (kind) {
      case INT1: return "bool";
      case INT8: return "char";
      case INT32: return "i32";
      case INT64: return "i64";
      case UINT8: return "u8";
      case UINT32: return "u32";
      case UINT64: return "u64";
      case FP64: return "f64";
    }
    return "unknown";
  }

  /// Compare two basic types for equality. Returns 0 if the types aren't equal,
  /// 1 if they are exactly equivelant, and 2 if they are suitable for implicit
  /// conversion. For example, enums and primitive integers are implicitly
  /// convertible.
  int compare(const Type *other) const override {
    if (const BasicType *otherType = dynamic_cast<const BasicType *>(other)) {
      if (kind == otherType->kind)
        return 1;

      if (isIntegerType() && otherType->isIntegerType())
        return 2;
    }
    // TODO: support enums
    return 0;
  }

  /// Returns a LLVM type equivelant to this basic type.
  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
    switch (kind) {
      case INT1: return llvm::Type::getInt1Ty(ctx);
      case INT8: return llvm::Type::getInt8Ty(ctx);
      case INT32: return llvm::Type::getInt32Ty(ctx);
      case INT64: return llvm::Type::getInt64Ty(ctx);
      case UINT8: return llvm::Type::getInt8Ty(ctx);
      case UINT32: return llvm::Type::getInt32Ty(ctx);
      case UINT64: return llvm::Type::getInt64Ty(ctx);
      case FP64: return llvm::Type::getDoubleTy(ctx);
    }
    return nullptr;
  }
};

/// Represents a function type. Function types are used to represent the type of
/// a function declaration. For example, `() -> i64`.
class FunctionType final : public Type {
  /// The return type of the function.
  const Type *returnType;

  /// The parameter types of the function.
  const vector<const Type *> paramTypes;

public:
  FunctionType(const Type *returnType, vector<const Type *> paramTypes)
      : returnType(returnType), paramTypes(paramTypes) {}

  /// Returns the return type of the function.
  const Type *getReturnType() const { return returnType; }

  /// Returns the type of a parameter at the given index.
  const Type *getParamType(size_t index) const { return paramTypes[index]; }

  /// Returns true if the function type returns an integer, and false otherwise.
  bool isIntegerType() const override { return returnType->isIntegerType(); }

  /// Returns true if the function type returns a floating point, and false
  /// otherwise.
  bool isFloatingPointType() const override { 
    return returnType->isFloatingPointType();
  }

  /// Returns the bit width of the function return type.
  unsigned getBitWidth() const override { return returnType->getBitWidth(); }

  /// Returns a string representation of the function type, including both the
  /// return type and each individual parameter type.
  string toString() const override {
    string str = "(";
    for (const Type *paramType : paramTypes) {
      str.append(paramType->toString() + ", ");
    }
  
    // Remove the last comma and space.
    if (str.size() > 1) {
      str.pop_back(); 
      str.pop_back();
    }
  
    return str.append(") -> " + returnType->toString());
  }

  /// Compare a function type with another type. Function types match if and
  /// only if all paremeters and the return type match. The return value of
  /// this function is never 2 due to explicitness of function types.
  int compare(const Type *other) const override {
    if (const FunctionType 
          *otherType = dynamic_cast<const FunctionType *>(other)) {
      if (returnType->compare(otherType->returnType) == 0)
        return 0;

      if (paramTypes.size() != otherType->paramTypes.size())
        return 0;

      for (size_t i = 0; i < paramTypes.size(); i++) {
        if (paramTypes[i]->compare(otherType->paramTypes[i]) == 0)
          return 0;
      }

      return 1;
    }
    return 0;
  }

  /// Returns a LLVM FunctionType equivelant of this function type.
  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
    vector<llvm::Type *> paramTypes;
    for (const Type *paramType : this->paramTypes) {
      paramTypes.push_back(paramType->toLLVMType(ctx));
    }

    return llvm::FunctionType::get(returnType->toLLVMType(ctx), 
        paramTypes, false);
  }
};

} // namespace artus

#endif // ARTUS_SEMA_TYPE_H
