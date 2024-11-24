#ifndef ARTUS_SEMA_TYPE_H
#define ARTUS_SEMA_TYPE_H

#include <string>
#include <vector>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

#include "../Core/Logger.h"

using std::string;
using std::vector;

namespace artus {

/// Base class for all Types and their derivatives. Often nested in an 
/// expression node or typed declaration.
class Type {
public:
  virtual ~Type() = default;

  /// Returns true if the type is absolute and not a qualified reference.
  virtual bool isAbsolute() const { return true; }

  /// Returns true if the most shallow type is definitively a boolean type.
  virtual bool isBooleanType() const = 0;

  /// Returns true if the type is definitively an integer type, or can be
  /// evaluted to otherwise, and false otherwise.
  virtual bool isIntegerType() const = 0;

  /// Returns true if the type is definitively a floating point type, or can be
  /// evaluted to otherwise, and false otherwise.
  virtual bool isFloatingPointType() const = 0;

  /// Returns true if the type is definitively a string type, and false
  /// otherwise.
  virtual bool isStringType() const = 0;

  /// Returns true if the type is a pointer type, and false otherwise.
  virtual bool isPointerType() const = 0;

  /// Returns true if the type is an array type, and false otherwise.
  virtual bool isArrayType() const = 0;

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

  /// Returns true if this type can be casted into the given type, and false
  /// otherwise. Optionally, a `strict` flag can be passed to indicate that the
  /// cast must be exact.
  virtual bool canCastTo(const Type *other, bool strict = false) const = 0;

  /// Returns true if the type can be subscripted, and false otherwise.
  virtual bool canSubscript() const { return false; }

  /// Returns the equivelant LLVM type for the given type.
  virtual llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const = 0;
};

/// Represents a reference to a possibly qualified type. This type is used
/// prior to type resolution and assigned during sema.
class TypeRef final : public Type {
  /// The identifier of the type.
  const string identifier;

public:
  TypeRef(const string &identifier) : identifier(identifier) {}

  bool isAbsolute()          const override { return false; }
  bool isBooleanType()       const override { return false; }
  bool isIntegerType()       const override { return false; }
  bool isFloatingPointType() const override { return false; }
  bool isStringType()        const override { return false; }
  bool isPointerType()       const override { return false; }
  bool isArrayType()         const override { return false; }

  unsigned getBitWidth() const override { return 0; }

  string toString() const override { return identifier; }

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

/// Class to represent basic, built-in types, such as `i64` or `char`.
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

  /// The kind of basic type.
  const BasicTypeKind kind;

  /// Constructor for a basic type. Only called by the CContext class.
  BasicType(BasicTypeKind kind) : kind(kind) {}

public:
  /// Returns true if the basic type kind is a boolean.
  bool isBooleanType() const override { return kind == INT1; }

  /// Returns true if the basic type kind is an integer or a character, and
  /// false otherwise.
  bool isIntegerType() const override { return kind <= UINT64; }

  /// Returns true if the basic type kind is a floating point, and false
  /// otherwise.
  bool isFloatingPointType() const override { return kind == FP64; }

  /// Returns true if the basic type kind is a string, and false otherwise.
  bool isStringType() const override { return kind == STR; }

  /// Returns true if the type is a pointer type, and false otherwise.
  bool isPointerType() const override { return false; }

  /// Returns true if the type is an array type, and false otherwise.
  bool isArrayType() const override { return false; }

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
      case STR: return 0;
    }
    return 0;
  }

  /// Returns the kind of this basic type.
  BasicType::BasicTypeKind getKind() const { return kind; }

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
      case STR: return "str";
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

      return 2;
    }
    // TODO: support enums
    return 0;
  }

  /// Returns true if this basic type can be casted into the given type, and
  /// false otherwise. Optionally, a `strict` flag can be passed to indicate
  /// that the cast must be exact.
  bool canCastTo(const Type *other, bool strict = false) const override {
    if (const BasicType *otherType = dynamic_cast<const BasicType *>(other)) {
      switch (strict ? 1 : 2) {
        case 1: return this->getBitWidth() == otherType->getBitWidth();
        case 2: return this->getBitWidth() <= otherType->getBitWidth();
      }
    }

    return false;
  }

  bool canSubscript() const override { return kind == STR; }

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
      case STR: return llvm::Type::getInt8Ty(ctx)->getPointerTo();
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

  /// Returns the return type of the function type.
  const Type *getReturnType() const { return returnType; }

  /// Returns the parameter types of the function type.
  const vector<const Type *> getParamTypes() const { return paramTypes; }

  /// Returns the type of a parameter at the given index.
  const Type *getParamType(size_t index) const { return paramTypes[index]; }

  bool isBooleanType() const override { return false; }

  /// Returns true if the function type returns an integer, and false otherwise.
  bool isIntegerType() const override { return returnType->isIntegerType(); }

  /// Returns true if the function type returns a floating point, and false
  /// otherwise.
  bool isFloatingPointType() const override 
  { return returnType->isFloatingPointType(); }

  /// Returns true if the function type returns a string type, and false 
  /// otherwise.
  bool isStringType() const override { return returnType->isStringType(); }

  /// Returns true if the type is a pointer type, and false otherwise.
  bool isPointerType() const override { return false; }

  /// Returns true if the type is an array type, and false otherwise.
  bool isArrayType() const override { return false; }

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

  /// Returns true if this function type can be casted into the given type, and
  /// false otherwise. Optionally, a `strict` flag can be passed to indicate
  /// that the cast must be exact.
  bool canCastTo(const Type *other, bool strict = false) const override {
    if (const FunctionType *otherType = dynamic_cast<const FunctionType *>(other)) {
      if (returnType->canCastTo(otherType->returnType, strict)) {
        for (size_t i = 0; i < paramTypes.size(); i++) {
          if (!paramTypes[i]->canCastTo(otherType->paramTypes[i], strict))
            return false;
        }
        return true;
      }
    }

    return false;
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

/// Represents a pointer type. Pointers types represent the type of the value 
/// contained at a memory address. For example, `*i64`.
class PointerType final : public Type {
  /// The type of the pointer.
  const Type *pointeeType;

public:
  PointerType(const Type *pointeeType) : pointeeType(pointeeType) {}

  /// Returns if the pointee type is absolute.
  bool isAbsolute() const override { return pointeeType->isAbsolute(); }

  /// Returns the type of the pointer.
  const Type *getPointeeType() const { return pointeeType; }

  bool isBooleanType() const override { return false; }

  /// Returns true if the pointee type is an integer, and false otherwise.
  bool isIntegerType() const override { return pointeeType->isIntegerType(); }

  /// Returns true if the pointee type is a floating point, and false otherwise.
  bool isFloatingPointType() const override 
  { return pointeeType->isFloatingPointType(); }

  /// Returns true if the pointee type is a string type, and false otherwise.
  bool isStringType() const override { return pointeeType->isStringType(); }

  /// Returns true if the type is a pointer type, and false otherwise.
  bool isPointerType() const override { return true; }

  /// Returns true if the type is an array type, and false otherwise.
  bool isArrayType() const override { return false; }

  /// Returns the bit width of the pointee type.
  unsigned getBitWidth() const override { return pointeeType->getBitWidth(); }

  /// Returns a string representation of the pointee type.
  string toString() const override { return '#' + pointeeType->toString(); }

  /// Compare a pointer type with another type. Pointer types match if and only
  /// if the pointee types match. The return value of this function is never 2
  /// due to explicitness of pointer types.
  int compare(const Type *other) const override {
    if (const PointerType *otherType = dynamic_cast<const PointerType *>(other)) {
      if (pointeeType->compare(otherType->pointeeType) == 0)
        return 0;

      return 1;
    }
    return 0;
  }

  /// Returns true if this pointer type can be casted into the given type, and
  /// false otherwise. Optionally, a `strict` flag can be passed to indicate
  /// that the cast must be exact.
  bool canCastTo(const Type *other, bool strict = false) const override {
    if (const PointerType *otherType = dynamic_cast<const PointerType *>(other)) {
      return pointeeType->canCastTo(otherType->pointeeType, strict);
    }

    return false;
  }

  bool canSubscript() const override { return pointeeType->isArrayType(); }

  /// Returns a LLVM PointerType equivelant of this pointer type.
  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override
  { return llvm::PointerType::get(pointeeType->toLLVMType(ctx), 0); }
};

/// Represents an array type. Array types are used to represent the type of an
/// array declaration. For example, `i64[10]`.
class ArrayType final : public Type {
  /// The type of the array.
  const Type *elementType;

  /// The size of the array.
  const size_t size;

public:
  ArrayType(const Type *elementType, size_t size)
      : elementType(elementType), size(size) {}

  /// Returns if the element type is absolute.
  bool isAbsolute() const override { return elementType->isAbsolute(); }

  /// Returns the type of the array.
  const Type *getElementType() const { return elementType; }

  /// Returns the size of the array.
  size_t getSize() const { return size; }

  bool isBooleanType() const override { return false; }

  /// Returns true if the array element type is an integer.
  bool isIntegerType() const override { return elementType->isIntegerType(); }

  /// Returns true if the array element type is a floating point.
  bool isFloatingPointType() const override 
  { return elementType->isFloatingPointType(); }

  /// Returns true if the array element type is a string type.
  bool isStringType() const override { return elementType->isStringType(); }

  /// Returns true if the type is a pointer type, and false otherwise.
  bool isPointerType() const override { return false; }

  /// Returns true if the type is an array type, and false otherwise.
  bool isArrayType() const override { return true; }

  /// Returns the bit width of the array type.
  unsigned getBitWidth() const override { return elementType->getBitWidth(); }

  /// Returns a string representation of the array type.
  string toString() const override 
  { return elementType->toString() + '[' + std::to_string(size) + ']'; }

  /// Compare an array type with another type. Array types match if and only if
  /// the element types and sizes match. The return value of this function is
  /// never 2 due to explicitness of array types.
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

  /// Returns true if this array type can be casted into the given type, and
  /// false otherwise. Optionally, a `strict` flag can be passed to indicate
  /// that the cast must be exact.
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

  /// Returns a LLVM ArrayType equivelant of this array type.
  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override 
  { return llvm::ArrayType::get(elementType->toLLVMType(ctx), size); }
};

/// Base class for all source-defined types (structs, enums, etc.).
class DefinedType : public Type {
protected:
  /// The name of the defined type.
  const string name;

public:
  DefinedType(const string &name) : name(name) {}

  bool isBooleanType()       const override { return false; }
  bool isIntegerType()       const override { return false; }
  bool isFloatingPointType() const override { return false; }
  bool isStringType()        const override { return false; }
  bool isPointerType()       const override { return false; }
  bool isArrayType()         const override { return false; }

  /// Returns a string representation of the struct type.
  string toString() const override { return name; }
};

/// Represesnts a defined struct type.
class StructType final : public DefinedType {
  /// The fields of the struct type.
  const vector<const Type *> fields;

public:
  StructType(const string &name, vector<const Type *> fields)
      : DefinedType(name), fields(fields) {}

  /// Returns true if all field types are absolute.
  bool isAbsolute() const override {
    for (const Type *field : fields) {
      if (!field->isAbsolute())
        return false;
    }
    return true;
  }

  /// Returns the number of fields in the struct type.
  size_t getNumFields() const { return fields.size(); }

  /// Returns the type of a field at the given index.
  const Type *getFieldType(size_t i) const 
  { return i < fields.size() ? fields[i] : nullptr; }

  /// Returns the bit width of the struct type.
  unsigned getBitWidth() const override {
    unsigned width = 0;
    for (const Type *field : fields) {
      width += field->getBitWidth();
    }
    return width;
  }

  /// Compare a struct type with another type. Struct types match if and only if
  /// the names and fields match. The return value of this function is never 2
  /// due to explicitness of struct types.
  int compare(const Type *other) const override {
    if (const StructType *otherType = dynamic_cast<const StructType *>(other)) {
      return name == otherType->name;
    }
    return 0;
  }

  /// Returns true if this struct type can be casted into the given type, and
  /// false otherwise. Optionally, a `strict` flag can be passed to indicate
  /// that the cast must be exact.
  bool canCastTo(const Type *other, bool strict = false) const override 
  { return false; }

  /// Returns a LLVM StructType equivelant of this struct type.
  llvm::Type *toLLVMType(llvm::LLVMContext &ctx) const override {
    vector<llvm::Type *> fieldTypes;
    for (const Type *field : fields) {
      fieldTypes.push_back(field->toLLVMType(ctx));
    }

    return llvm::StructType::create(fieldTypes, name);
  }
};

} // namespace artus

#endif // ARTUS_SEMA_TYPE_H
