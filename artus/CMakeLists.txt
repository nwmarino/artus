cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_CXX_STANDARD 20)
project(artus)

add_subdirectory(lib)

target_include_directories(artus_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(artus_lexer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Lex
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(artus artus.cpp)

target_link_libraries(artus 
    PRIVATE 
        artus_core artus_lexer
)
