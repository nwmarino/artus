#ifndef ARTUS_SEMA_TYPE_H
#define ARTUS_SEMA_TYPE_H

#include <vector>

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

  /// TODO: Returns the equivelant LLVM type for the given type.
  /// virtual llvm::Type *toLLVMType() const = 0;
};

/// Class to represent basic, built-in types, such as `i64` or `char`.
class BasicType final : public Type {
  friend class CContext;

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
  inline bool isIntegerType() const override { return kind <= UINT64; }

  /// Returns true if the basic type kind is a floating point, and false
  /// otherwise.
  inline bool isFloatingPointType() const override { return kind == FP64; }

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

  /// llvm::Type *toLLVMType() const override;
};

} // namespace artus

#endif // ARTUS_SEMA_TYPE_H
