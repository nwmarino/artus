cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_CXX_STANDARD 20)
project(artus)

add_subdirectory(lib)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(artus_ast
    PUBLIC
        ${INCLUDE_DIR}/AST
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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(artus 
    artus.cpp 
    lib/AST/ASTPrinter.cpp 
    lib/AST/Stmt.cpp
    lib/Core/Context.cpp
    lib/Core/Logger.cpp
    lib/Lex/Lexer.cpp
    lib/Parse/ParseDecl.cpp
    lib/Parse/ParseExpr.cpp
    lib/Parse/ParseStmt.cpp
)

include_directories(${INCLUDE_DIR})

target_link_libraries(artus
    PRIVATE 
        artus_ast 
        artus_core 
        artus_lexer 
        artus_parser
)
