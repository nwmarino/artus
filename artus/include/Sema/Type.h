#ifndef ARTUS_SEMA_TYPE_H
#define ARTUS_SEMA_TYPE_H

#include <string>
#include <vector>

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

  /// Returns a string representation of the type. This identifier is identical
  /// to the identifier parsed.
  virtual string toString() const = 0;

  /// TODO: Returns the equivelant LLVM type for the given type.
  /// virtual llvm::Type *toLLVMType() const = 0;
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
  }

  /// llvm::Type *toLLVMType() const override;
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

  /// Returns true if the function type returns an integer, and false otherwise.
  bool isIntegerType() const override { return returnType->isIntegerType(); }

  /// Returns true if the function type returns a floating point, and false
  /// otherwise.
  bool isFloatingPointType() const override { 
    return returnType->isFloatingPointType();
  }

  /// Returns a string representation of the function type, including both the
  /// return type and each individual parameter type.
  string toString() const override {
    string str = "(";
    for (const Type *paramType : paramTypes) {
      str.append(paramType->toString() + ", ");
    }
  
    // Remove the last comma and space.
    if (str != "(")
      str.pop_back(); str.pop_back();
  
    return str.append(") -> " + returnType->toString());
  } 

  /// llvm::Type *toLLVMType() const override;
};

} // namespace artus

#endif // ARTUS_SEMA_TYPE_H
