cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_CXX_STANDARD 20)
project(artus)

add_subdirectory(lib)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

message(STATUS "LLVM includes: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")
message(STATUS "LLVM tools: ${LLVM_TOOLS_BINARY_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(artus_ast
    PUBLIC
        ${INCLUDE_DIR}/AST
)

target_include_directories(artus_codegen
    PUBLIC
        ${INCLUDE_DIR}/Codegen
)

target_include_directories(artus_core
    PUBLIC
        ${INCLUDE_DIR}/Core
)

target_include_directories(artus_lexer
    PUBLIC
        ${INCLUDE_DIR}/Lexer
)

target_include_directories(artus_parser
    PUBLIC
        ${INCLUDE_DIR}/Parse
)

target_include_directories(artus_sema
    PUBLIC
        ${INCLUDE_DIR}/Sema
) 

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(artus 
    artus.cpp 
    lib/AST/ASTPrinter.cpp 
    lib/AST/Stmt.cpp
    lib/Codegen/Codegen.cpp
    lib/Core/Context.cpp
    lib/Core/Logger.cpp
    lib/Lex/Lexer.cpp
    lib/Parse/ParseDecl.cpp
    lib/Parse/ParseExpr.cpp
    lib/Parse/ParseStmt.cpp
    lib/Sema/Sema.cpp
)

include_directories(${INCLUDE_DIR})

target_link_libraries(artus
    PRIVATE 
        artus_ast
        artus_codegen
        artus_core 
        artus_lexer 
        artus_parser
        artus_sema
)

llvm_config(artus USE_SHARED core irreader support clang)
