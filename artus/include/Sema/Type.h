//>==- Type.h -------------------------------------------------------------==<//
//
// This header files defines the type system and types used and recognized by
// the language.
//
//>==----------------------------------------------------------------------==<//

#ifndef ARTUS_SEMA_TYPE_H
#define ARTUS_SEMA_TYPE_H

#include <string>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

namespace artus {

/// Forward declarations.
class EnumType;

/// Base class for all Types and their derivatives. Often nested in an 
/// expression node or typed declaration.
class Type {
public:
  virtual ~Type() = default;

  /// \returns `true` if this type is absolute and not a qualified reference.
  virtual bool isAbsolute() const { return true; }

  /// \returns `true` if this most shallow type is definitively a boolean type.
  virtual bool isBooleanType() const = 0;

  /// \returns `true` if this type is definitively an integer type, or can be
  /// evaluted to otherwise.
  virtual bool isIntegerType() const = 0;

  /// \returns `true` if this type is definitively a floating point type, or can be
  /// evaluted to otherwise.
  virtual bool isFloatingPointType() const = 0;

  /// \returns `true` if this type is definitively a numerical type, or can be
  /// evaluated to otherwise.
  virtual bool isNumericalType() const = 0;

  /// \returns `true` if this type is definitively a string type, and false
  /// otherwise.
  virtual bool isStringType() const = 0;

  /// \returns `true` if this type is a pointer type.
  virtual bool isPointerType() const = 0;

  /// \returns `true` if this type is an array type.
  virtual bool isArrayType() const = 0;

  /// \returns `true` if this type is a struct type and has members.
  virtual bool isStructType() const { return false; }

  /// \returns The bit width of this type. For example, `i64` returns 64.
  virtual unsigned getBitWidth() const = 0;

  /// \returns The string representation of this type. This representation is 
  /// identical to the identifier parsed.
  virtual const std::string toString() const = 0;

  /// Compare two types for equality. Returns 0 if the types are not equal, 1
  /// if they are exactly equivelant, and 2 if they are suitable for implicit
  /// conversion.
  virtual int compare(const Type *other) const = 0;

  /// \returns `true` if this type can be casted into the given type, and false
  /// otherwise. Optionally, a `strict` flag can be passed to indicate that the
  /// cast must be exact.
  virtual bool canCastTo(const Type *other, bool strict = false) const = 0;

  /// \returns `true` if this type can be subscripted.
  virtual bool canSubscript() const { return false; }

  /// \returns The equivelant LLVM type for this type.
  virtual llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const = 0;
};

/// Represents a reference to a possibly qualified type.
class TypeRef final : public Type {
  /// The identifier of the type.
  const std::string identifier;

public:
  TypeRef(const std::string &identifier) : identifier(identifier) {}

  bool isAbsolute() const override { return false; }

  bool isBooleanType() const override { return false; }

  bool isIntegerType() const override { return false; }

  bool isFloatingPointType() const override { return false; }

  bool isNumericalType() const override { return false; }

  bool isStringType() const override { return false; }

  bool isPointerType() const override { return false; }

  bool isArrayType() const override { return false; }

  unsigned getBitWidth() const override { return 0; }

  const std::string toString() const override { return identifier; }

  int compare(const Type *other) const override {
    if (const TypeRef *otherType = dynamic_cast<const TypeRef *>(other)) {
      if (identifier == otherType->identifier)
        return 1;

      return 2;
    }
    return 0;
  }

  bool canCastTo(const Type *other, bool strict = false) const override 
  { return false; }

  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override 
  { return nullptr; }
};

/// Represents built-in types, such as `i64` or `char`.
class BasicType final : public Type {
  friend class Context;

public:
  /// Possible kinds of basic types.
  enum BasicTypeKind {
    INT1 = 0,
    INT8,
    INT32,
    INT64,
    UINT8,
    UINT32,
    UINT64,
    FP64,
    STR,
  };

private:
  /// The kind of basic type this type represents.
  const BasicTypeKind kind;

  /// Constructor for a basic type. Only called by the CContext class.
  BasicType(BasicTypeKind kind) : kind(kind) {}

public:
  /// \returns `true` if the basic type kind is a boolean.
  bool isBooleanType() const override { return kind == INT1; }

  /// \returns `true` if the basic type kind is an integer or a character.
  bool isIntegerType() const override { return kind <= UINT64; }

  /// \returns `true` if the basic type kind is a floating point.
  bool isFloatingPointType() const override { return kind == FP64; }

  /// \returns `true` if the basic type is a non-str string.
  bool isNumericalType() const override { return kind <= FP64; }

  /// \returns `true` if the basic type kind is a string.
  bool isStringType() const override { return kind == STR; }

  /// \returns `true` if the type is a pointer type.
  bool isPointerType() const override { return false; }

  /// \returns `true` if the type is an array type.
  bool isArrayType() const override { return false; }

  /// \returns the bit width of this basic type. For example, an `i64` would
  /// return 64.
  unsigned getBitWidth() const override {
    switch (kind) {
    case INT1: 
      return 1;
    case INT8: 
      return 8;
    case INT32: 
      return 32;
    case INT64: 
      return 64;
    case UINT8: 
      return 8;
    case UINT32: 
      return 32;
    case UINT64: 
      return 64;
    case FP64: 
      return 64;
    case STR:
      return 0;
    }
  }

  /// \returns The kind of this basic type.
  BasicType::BasicTypeKind getKind() const { return kind; }

  /// \returns The string representation of the basic type.
  const std::string toString() const override {
    switch (kind) {
    case INT1: 
      return "bool";
    case INT8: 
      return "char";
    case INT32: 
      return "i32";
    case INT64: 
      return "i64";
    case UINT8: 
      return "u8";
    case UINT32: 
      return "u32";
    case UINT64: 
      return "u64";
    case FP64: 
      return "f64";
    case STR: 
      return "str";
    }
  }

  /// Compare two basic types for equality. \returns 0 if the types are not
  /// equal, 1 if they are exactly equivelant, and 2 if they are suitable for 
  /// implicit conversion.
  int compare(const Type *other) const override {
    if (const BasicType *otherType = dynamic_cast<const BasicType *>(other)) {
      if (kind == otherType->kind)
        return 1;

      // Cannot cast strings to and from the other basic types.
      if (kind != otherType->kind && (kind == STR || otherType->kind == STR))
        return 0;

      return 2;
    } else if (this->isIntegerType() && other->isIntegerType())
      return this->getBitWidth() == other->getBitWidth() ? 1 : 2;
  
    return 0;
  }

  /// \returns `true` if this basic type can be casted into the given type, and
  /// `false` otherwise. Optionally, a `strict` flag can be passed to indicate
  /// that the cast must be exact.
  bool canCastTo(const Type *other, bool strict = false) const override {
    if (const BasicType *otherType = dynamic_cast<const BasicType *>(other)) {
      switch (strict ? 1 : 2) {
      case 1: 
        return this->getBitWidth() == otherType->getBitWidth();
      case 2: 
        return this->getBitWidth() <= otherType->getBitWidth();
      }
    }

    return false;
  }

  bool canSubscript() const override { return kind == STR; }

  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
    switch (kind) {
    case INT1: 
      return llvm::Type::getInt1Ty(ctx);
    case INT8: 
      return llvm::Type::getInt8Ty(ctx);
    case INT32: 
      return llvm::Type::getInt32Ty(ctx);
    case INT64: 
      return llvm::Type::getInt64Ty(ctx);
    case UINT8:
      return llvm::Type::getInt8Ty(ctx);
    case UINT32:
      return llvm::Type::getInt32Ty(ctx);
    case UINT64: 
      return llvm::Type::getInt64Ty(ctx);
    case FP64: 
      return llvm::Type::getDoubleTy(ctx);
    case STR: 
      return llvm::Type::getInt8Ty(ctx)->getPointerTo();
    }
  }
};

/// Represents a function type. 
///
/// Function types are used to represent the type of a function declaration. 
/// For example, `() -> i64`.
class FunctionType final : public Type {
  /// The return type of the function.
  const Type *returnType;

  /// The parameter types of the function.
  const std::vector<const Type *> paramTypes;

public:
  FunctionType(const Type *returnType, std::vector<const Type *> paramTypes)
      : returnType(returnType), paramTypes(paramTypes) {}
    
  /// Returns true if all parameters are absolute, as well as the return type.
  bool isAbsolute() const override {
    if (!returnType->isAbsolute())
      return false;

    for (const Type *paramType : paramTypes) {
      if (!paramType->isAbsolute())
        return false;
    }

    return true;
  }

  /// \returns The return type of this function type.
  const Type *getReturnType() const { return returnType; }

  /// \returns The parameter types of this function type.
  const std::vector<const Type *> getParamTypes() const { return paramTypes; }

  /// \returns The type of the parameter at index \p index.
  const Type *getParamType(std::size_t index) const 
  { return paramTypes[index]; }

  bool isBooleanType() const override { return false; }

  /// \returns `true` if the function type returns an integer.
  bool isIntegerType() const override { return returnType->isIntegerType(); }

  /// \returns `true` if the function type returns a floating point.
  bool isFloatingPointType() const override 
  { return returnType->isFloatingPointType(); }

  /// \returns `true` if the function type returns a numerical type.
  bool isNumericalType() const override 
  { return returnType->isNumericalType(); }

  /// \returns `true` if the function type returns a string type.
  bool isStringType() const override { return returnType->isStringType(); }

  bool isPointerType() const override { return false; }

  bool isArrayType() const override { return false; }

  /// \returns The bit width of the function return type.
  unsigned getBitWidth() const override { return returnType->getBitWidth(); }

  const std::string toString() const override {
    std::string str = "(";
    for (const Type *paramType : paramTypes)
      str.append(paramType->toString() + ", ");
  
    // Remove the last comma and space.
    if (str.size() > 1) {
      str.pop_back(); 
      str.pop_back();
    }
  
    return str.append(") -> " + returnType->toString());
  }

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

  bool canCastTo(const Type *other, bool strict = false) const override 
  { return false; /* Function types cannot be casted to one another. */ }

  /// Returns a LLVM FunctionType equivelant of this function type.
  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
    std::vector<llvm::Type *> paramTypes;
    for (const Type *paramType : this->paramTypes)
      paramTypes.push_back(paramType->toLLVMType(ctx));

    return llvm::FunctionType::get(returnType->toLLVMType(ctx), paramTypes, 
        false);
  }
};

/// Represents a pointer type. 
///
/// Pointers types represent the type of the value contained at a memory 
/// address. For example, `*i64`.
class PointerType final : public Type {
  /// The type of the pointer.
  const Type *pointeeType;

public:
  PointerType(const Type *pointeeType) : pointeeType(pointeeType) {}

  /// \returns `true` if the pointee type is absolute.
  bool isAbsolute() const override { return pointeeType->isAbsolute(); }

  /// \returns The type of the pointee.
  const Type *getPointeeType() const { return pointeeType; }

  bool isBooleanType() const override { return false; }

  /// \returns `true` if the pointee type is an integer.
  bool isIntegerType() const override { return pointeeType->isIntegerType(); }

  /// \returns `true` if the pointee type is a floating point.
  bool isFloatingPointType() const override
  { return pointeeType->isFloatingPointType(); }

  /// \returns `true` if the pointee type is a numerical type.
  bool isNumericalType() const override 
  { return pointeeType->isNumericalType(); }

  /// \returns `true` if the pointee type is a string type.
  bool isStringType() const override { return pointeeType->isStringType(); }

  bool isPointerType() const override { return true; }

  bool isArrayType() const override { return false; }

  /// \returns The bit width of the pointee type.
  unsigned getBitWidth() const override { return pointeeType->getBitWidth(); }

  const std::string toString() const override 
  { return '#' + pointeeType->toString(); }

  int compare(const Type *other) const override {
    if (const PointerType *otherTy = dynamic_cast<const PointerType *>(other)) {
      if (pointeeType->compare(otherTy->pointeeType) == 0)
        return 0;

      return 1;
    }
    return 0;
  }

  bool canCastTo(const Type *other, bool strict = false) const override {
    if (const PointerType *otherTy = dynamic_cast<const PointerType *>(other))
      return pointeeType->canCastTo(otherTy->pointeeType, strict);

    return false;
  }

  bool canSubscript() const override { return pointeeType->isArrayType(); }

  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override
  { return llvm::PointerType::get(pointeeType->toLLVMType(ctx), 0); }
};

/// Represents an array type. Array types are used to represent the type of an
/// array declaration. For example, `i64[10]`.
class ArrayType final : public Type {
  /// The type of the array.
  const Type *elementType;

  /// The size of the array.
  const std::size_t size;

public:
  ArrayType(const Type *elementType, std::size_t size)
      : elementType(elementType), size(size) {}

  /// \returns `true` if the element type is absolute.
  bool isAbsolute() const override { return elementType->isAbsolute(); }

  /// \returns The type of the array.
  const Type *getElementType() const { return elementType; }

  /// \returns The size of the array.
  std::size_t getSize() const { return size; }

  bool isBooleanType() const override { return false; }

  /// \returns `true` if the array element type is an integer.
  bool isIntegerType() const override { return elementType->isIntegerType(); }

  /// \returns `true` if the array element type is a floating point.
  bool isFloatingPointType() const override 
  { return elementType->isFloatingPointType(); }

  /// \returns `true` if the array element type is a numerical type.
  bool isNumericalType() const override 
  { return elementType->isNumericalType(); }

  /// \returns `true` if the array element type is a string type.
  bool isStringType() const override { return elementType->isStringType(); }

  bool isPointerType() const override { return false; }

  bool isArrayType() const override { return true; }

  /// \returns The bit width of the array element type.
  unsigned getBitWidth() const override { return elementType->getBitWidth(); }

  const std::string toString() const override 
  { return elementType->toString() + '[' + std::to_string(size) + ']'; }

  int compare(const Type *other) const override {
    if (const ArrayType *otherType = dynamic_cast<const ArrayType *>(other)) {
      if (elementType->compare(otherType->elementType) == 0)
        return 0;

      if (size != otherType->size)
        return 0;

      return 1;
    }
    return 0;
  }

  bool canCastTo(const Type *other, bool strict = false) const override {
    if (const ArrayType *otherType = dynamic_cast<const ArrayType *>(other)) {
      if (elementType->canCastTo(otherType->elementType, strict)) {
        if (size == otherType->size)
          return true;
      }
    }

    return false;
  }

  bool canSubscript() const override { return true; }

  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override 
  { return llvm::ArrayType::get(elementType->toLLVMType(ctx), size); }
};

/// Base class for all source-defined types (structs, enums, etc.).
class DefinedType : public Type {
protected:
  /// The name of the defined type.
  const std::string name;

public:
  DefinedType(const std::string &name) : name(name) {}

  bool isBooleanType() const override { return false; }

  bool isIntegerType() const override { return false; }

  bool isFloatingPointType() const override { return false; }

  bool isNumericalType() const override { return false; }

  bool isStringType() const override { return false; }

  bool isPointerType() const override { return false; }

  bool isArrayType() const override { return false; }

  const std::string toString() const override { return name; }
};

/// Represesnts a defined struct type.
class StructType final : public DefinedType {
  /// The fields of the struct type.
  const std::vector<const Type *> fields;

public:
  StructType(const std::string &name, std::vector<const Type *> fields)
      : DefinedType(name), fields(fields) {}

  bool isStructType() const override { return true; }

  /// \returns `true` if all field types are absolute.
  bool isAbsolute() const override {
    for (const Type *field : fields) {
      if (!field->isAbsolute())
        return false;
    }

    return true;
  }

  /// \returns The number of fields in the struct type.
  std::size_t getNumFields() const { return fields.size(); }

  /// \returns The type of a field at the given index.
  const Type *getFieldType(std::size_t i) const 
  { return i < fields.size() ? fields[i] : nullptr; }

  /// \returns The total bit width of the struct type.
  unsigned getBitWidth() const override {
    unsigned width = 0;
    for (const Type *field : fields)
      width += field->getBitWidth();
  
    return width;
  }

  int compare(const Type *other) const override {
    if (const StructType *otherType = dynamic_cast<const StructType *>(other))
      return name == otherType->name;

    return 0;
  }

  bool canCastTo(const Type *other, bool strict = false) const override 
  { return false; }

  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
    std::vector<llvm::Type *> fieldTypes;
    for (const Type *field : fields)
      fieldTypes.push_back(field->toLLVMType(ctx));

    return llvm::StructType::create(ctx, fieldTypes, name);
  }
};

/// Represents a defined enum type.
class EnumType final : public DefinedType {
  /// The values of the enum type.
  const std::vector<std::string> variants;

public:
  EnumType(const std::string &name, std::vector<std::string> variants)
      : DefinedType(name), variants(variants) {}

  bool isAbsolute() const override { return true; }

  bool isIntegerType() const override { return true; }

  bool isNumericalType() const override { return true; }

  unsigned getBitWidth() const override { return 64; }

  /// \returns The value of a variant in the enumeration.
  int getVariant(const std::string &variant) const {
    for (size_t i = 0; i < variants.size(); i++) {
      if (variants[i] == variant)
        return i;
    }
    
    return -1;
  }

  int compare(const Type *other) const override {
    if (const EnumType *otherType = dynamic_cast<const EnumType *>(other))
      return name == otherType->name;
    else if (this->isIntegerType() && other->isIntegerType())
      return this->getBitWidth() == other->getBitWidth() ? 1 : 2;

    return false;
  }

  bool canCastTo(const Type *other, bool strict = false) const override {
    if (other->isIntegerType()) {
      switch (strict ? 1 : 2) {
      case 1: 
        return this->getBitWidth() == other->getBitWidth();
      case 2: 
        return this->getBitWidth() <= other->getBitWidth();
      }
    }

    return false;
  }

  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
  { return llvm::Type::getInt64Ty(ctx); }}
};

} // end namespace artus

#endif // ARTUS_SEMA_TYPE_H
